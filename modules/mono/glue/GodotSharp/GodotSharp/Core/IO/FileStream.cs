using System;
using System.Buffers;
using System.IO;
using SystemFileAccess = System.IO.FileAccess;

#nullable enable

namespace Godot
{
    public class FileStream : Stream
    {
        private readonly FileAccess _file;

        private readonly SystemFileAccess _access;

        private bool _disposed;

        /// <summary>
        /// Gets a value that indicates whether the current stream supports seeking.
        /// </summary>
        /// <value>
        /// <see langword="true"/> if the stream supports seeking;
        /// <see langword="false"/> if the stream is closed.
        /// </value>
        public override bool CanSeek => _file.IsOpen();

        /// <summary>
        /// Gets a value that indicates whether the current stream supports reading.
        /// </summary>
        /// <value>
        /// <see langword="true"/> if the stream supports reading;
        /// <see langword="false"/> if the stream is closed or was opened with
        /// write-only access.
        /// </value>
        public override bool CanRead => (_access & SystemFileAccess.Read) != 0 && _file.IsOpen() && !_file.EofReached();

        /// <summary>
        /// Gets a value that indicates whether the current stream supports writing.
        /// </summary>
        /// <value>
        /// <see langword="true"/> if the stream supports writing;
        /// <see langword="false"/> if the stream is closed or was opened with
        /// read-only access.
        /// </value>
        public override bool CanWrite => (_access & SystemFileAccess.Write) != 0 && _file.IsOpen();

        /// <summary>
        /// Gets the length in bytes of the stream.
        /// </summary>
        /// <value>A long value representing the length of the stream in bytes.</value>
        /// <exception cref="ObjectDisposedException">
        /// The stream is closed.
        /// </exception>
        public override long Length
        {
            get
            {
                ThrowIfClosed();
                return checked((long)_file.GetLength());
            }
        }

        /// <summary>
        /// Gets or sets the current position of this stream.
        /// </summary>
        /// <value>The current position of this stream.</value>
        /// <exception cref="ObjectDisposedException">
        /// The stream is closed.
        /// </exception>
        public override long Position
        {
            get
            {
                ThrowIfClosed();
                return checked((long)_file.GetPosition());
            }
            set
            {
                ThrowIfClosed();
                _file.Seek(checked((ulong)value));
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FileStream"/> class for
        /// the specified file, with the specified read/write permission.
        /// </summary>
        /// <param name="file">
        /// The file that the current <see cref="FileStream"/> object will encapsulate.
        /// </param>
        /// <param name="access">
        /// A bitwise combination of the enumeration values that sets the
        /// <see cref="CanRead"/> and <see cref="CanWrite"/> properties of the
        /// <see cref="FileStream"/> object.
        /// </param>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="file"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="file"/> is closed.
        /// </exception>
        public FileStream(FileAccess file, SystemFileAccess access)
        {
            if (file == null)
            {
                throw new ArgumentNullException(nameof(file), "File cannot be null.");
            }

            if (!file.IsOpen())
            {
                throw new ArgumentException("Cannot access a closed file.", nameof(file));
            }

            _file = file;
            _access = access;
        }

        /// <summary>
        /// Releases the unmanaged resources used by the <see cref="FileStream"/>
        /// and optionally releases the managed resources.
        /// </summary>
        /// <param name="disposing">
        /// <see langword="true"/> to release both managed and unmanaged resources;
        /// <see langword="false"/> to release only unmanaged resources.
        /// </param>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                _file.Dispose();
                _disposed = true;
            }

            base.Dispose(disposing);
        }

        /// <summary>
        /// Clears buffers for this stream and causes any buffered data to be
        /// written to the file.
        /// </summary>
        public override void Flush()
        {
            _file.Flush();
        }

        /// <summary>
        /// Sets the length of this stream to the given value.
        /// </summary>
        /// <param name="value">The new length of the stream.</param>
        /// <exception cref="NotSupportedException">
        /// The stream does not support setting the length of a file.
        /// </exception>
        public override void SetLength(long value)
        {
            throw new NotSupportedException();
        }

        /// <summary>
        /// Sets the current position of this stream to the given value.
        /// </summary>
        /// <param name="offset">
        /// The point relative to <paramref name="origin"/> from which to
        /// begin seeking.
        /// </param>
        /// <param name="origin">
        /// Specifies the beginning, the end, or the current position as a
        /// reference point for <paramref name="offset"/>, using a value of
        /// type <see cref="SeekOrigin"/>.
        /// </param>
        /// <returns>The new position in the stream.</returns>
        /// <exception cref="ObjectDisposedException">
        /// The stream is closed.
        /// </exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <paramref name="origin"/> value is invalid.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error has occurred.
        /// </exception>
        public override long Seek(long offset, SeekOrigin origin)
        {
            ThrowIfClosed();

            switch (origin)
            {
                case SeekOrigin.Begin:
                    _file.Seek(checked((ulong)offset));
                    break;
                case SeekOrigin.Current:
                    _file.Seek(checked((ulong)(Position + offset)));
                    break;
                case SeekOrigin.End:
                    _file.SeekEnd(offset);
                    break;
                default:
                    throw new ArgumentOutOfRangeException(nameof(origin), origin, null);
            }

            _file.GetError().ThrowIfError();

            return Position;
        }

        /// <summary>
        /// Reads a block of bytes from the stream and writes the data in a given buffer.
        /// </summary>
        /// <param name="buffer">
        /// When this method returns, contains the specified byte array with the
        /// values between <paramref name="offset"/> and
        /// (<paramref name="offset"/> + <paramref name="count"/> - 1) replaced
        /// by the bytes read from the current source.
        /// </param>
        /// <param name="offset">
        /// The byte offset in <paramref name="buffer"/> at which the read bytes
        /// will be placed.</param>
        /// <param name="count">The maximum number of bytes to read.</param>
        /// <returns>
        /// The total number of bytes read into the buffer. This might be less
        /// than the number of bytes requested if that number of bytes are not
        /// currently available, or zero if the end of the stream is reached.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="buffer"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <paramref name="offset"/> or <paramref name="count"/> is negative.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="offset"/> and <paramref name="count"/> describe an
        /// invalid range in <paramref name="buffer"/>.
        /// </exception>
        /// <exception cref="ObjectDisposedException">
        /// The stream is closed.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error has occurred.
        /// </exception>
        public override int Read(byte[] buffer, int offset, int count)
        {
            ValidateReadWriteArgs(buffer, offset, count);

            return Read(buffer.AsSpan(offset, count));
        }

        /// <summary>
        /// Reads a sequence of bytes from the current file stream and advances
        /// the position within the file stream by the number of bytes read.
        /// </summary>
        /// <param name="buffer">
        /// A region of memory. When this method returns, the contents of this
        /// region are replaced by the bytes read from the current file stream.
        /// </param>
        /// <returns>
        /// The total number of bytes read into the buffer. This can be less
        /// than the number of bytes allocated in the buffer if that many bytes
        /// are not currently available, or zero if the end of the stream is reached.
        /// </returns>
        /// <exception cref="ObjectDisposedException">
        /// The stream is closed.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error has occurred.
        /// </exception>
        public override int Read(Span<byte> buffer)
        {
            ThrowIfClosed();

            int remaining = (int)(Length - Position);

            int size = Math.Min(buffer.Length, remaining);
            byte[] data = _file.GetBuffer(size);

            _file.GetError().ThrowIfError();

            data.CopyTo(buffer);

            return data.Length;
        }

        /// <summary>
        /// Writes a block of bytes to the file stream.
        /// </summary>
        /// <param name="buffer">
        /// The buffer containing data to write to the stream.
        /// </param>
        /// <param name="offset">
        /// The zero-based byte offset in array from which to begin copying
        /// bytes to the stream.
        /// </param>
        /// <param name="count">The maximum number of bytes to write.</param>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="buffer"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <paramref name="offset"/> or <paramref name="count"/> is negative.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="offset"/> and <paramref name="count"/> describe an
        /// invalid range in <paramref name="buffer"/>.
        /// </exception>
        /// <exception cref="ObjectDisposedException">
        /// The stream is closed.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error has occurred.
        /// </exception>
        public override void Write(byte[] buffer, int offset, int count)
        {
            ValidateReadWriteArgs(buffer, offset, count);

            int size = Math.Min(buffer.Length - offset, count);

            if (offset == 0 && buffer.Length <= count)
            {
                _file.StoreBuffer(buffer);
            }
            else
            {
                var data = new byte[size];

                Array.Copy(sourceArray: buffer, sourceIndex: offset, destinationArray: data, destinationIndex: 0, length: size);

                _file.StoreBuffer(data);
            }

            _file.GetError().ThrowIfError();
        }

        /// <summary>
        /// Writes a sequence of bytes from a read-only span to the current file
        /// stream and advances the current position within this file stream by
        /// the number of bytes written.
        /// </summary>
        /// <param name="buffer">
        /// A region of memory. This method copies the contents of this region
        /// to the current file stream.
        /// </param>
        /// <exception cref="ObjectDisposedException">
        /// The stream is closed.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error has occurred.
        /// </exception>
        public override void Write(ReadOnlySpan<byte> buffer)
        {
            ThrowIfClosed();

            byte[] sharedBuffer = ArrayPool<byte>.Shared.Rent(buffer.Length);
            try
            {
                buffer.CopyTo(sharedBuffer);
                _file.StoreBuffer(sharedBuffer);
                _file.GetError().ThrowIfError();
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(sharedBuffer);
            }
        }

        /// <summary>
        /// Validates the arguments of a ReadWrite method.
        /// </summary>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="buffer"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <paramref name="offset"/> or <paramref name="count"/> is negative.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="offset"/> and <paramref name="count"/> describe an
        /// invalid range in <paramref name="buffer"/>.
        /// </exception>
        /// <exception cref="ObjectDisposedException">
        /// The stream is closed.
        /// </exception>
        private void ValidateReadWriteArgs(byte[] buffer, int offset, int count)
        {
            if (buffer == null)
            {
                throw new ArgumentNullException(nameof(buffer), "Buffer cannot be null.");
            }
            if (offset < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(offset), "Non-negative number required.");
            }
            if (count < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(count), "Non-negative number required.");
            }

            if (buffer.Length < offset)
            {
                throw new ArgumentException("Offset and length are out of bounds.");
            }
            if (buffer.Length < offset + count)
            {
                throw new ArgumentException("Count is greater than the number of elements from offset to the end of the source collection.");
            }

            ThrowIfClosed();
        }

        private void ThrowIfClosed()
        {
            if (_disposed || !_file.IsOpen())
            {
                throw new ObjectDisposedException("Cannot access a closed file.");
            }
        }
    }
}
