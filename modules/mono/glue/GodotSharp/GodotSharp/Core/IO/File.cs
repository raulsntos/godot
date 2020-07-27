using System;
using System.Buffers;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using SystemFileAccess = System.IO.FileAccess;

#nullable enable

namespace Godot
{
    public partial class File
    {
        private const int BufferSize = 4096;

        /// <summary>
        /// Opens an existing UTF-8 encoded text file for reading.
        /// </summary>
        /// <param name="path">The file to be opened for reading.</param>
        /// <returns>A <see cref="StreamReader"/> on the specified path.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static StreamReader OpenText(string path)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            var stream = Open(path, SystemFileAccess.Read);

            return new StreamReader(stream);
        }

        /// <summary>
        /// Creates or opens a file for writing UTF-8 encoded text.
        /// If the file already exists, its contents are overwritten.
        /// </summary>
        /// <param name="path">The file to be opened for writing.</param>
        /// <returns>
        /// A <see cref="StreamWriter"/> that writes to the specified file using
        /// UTF-8 encoding.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while creating the file.
        /// </exception>
        public static StreamWriter CreateText(string path)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            var stream = Open(path, SystemFileAccess.Write);

            return new StreamWriter(stream);
        }

        /// <summary>
        /// Copies an existing file to a new file. Overwriting a file of the
        /// same name is not allowed.
        /// </summary>
        /// <param name="sourceFileName">The file to copy.</param>
        /// <param name="destFileName">
        /// The name of the destination file. This cannot be a directory or
        /// an existing file.
        /// </param>
        /// <exception cref="ArgumentException">
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// is a zero-length string, contains only white space, or contains one
        /// or more invalid characters.
        /// -or-
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// specifies a directory.
        /// </exception>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="IOException">
        /// A file with the same name and location specified by
        /// <paramref name="destFileName"/> already exists.
        /// -or-
        /// An I/O error has occurred.
        /// </exception>
        public static void Copy(string sourceFileName, string destFileName)
        {
            Copy(sourceFileName, destFileName, overwrite: false);
        }

        /// <summary>
        /// Copies an existing file to a new file. Overwriting a file of the
        /// same name is allowed.
        /// </summary>
        /// <param name="sourceFileName">The file to copy.</param>
        /// <param name="destFileName">
        /// The name of the destination file. This cannot be a directory.
        /// </param>
        /// <param name="overwrite">
        /// <see langword="true"/> if the destination file can be overwritten;
        /// otherwise, <see langword="false"/>.
        /// </param>
        /// <exception cref="ArgumentException">
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// is a zero-length string, contains only white space, or contains one
        /// or more invalid characters.
        /// -or-
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// specifies a directory.
        /// </exception>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="IOException">
        /// A file with the same name and location specified by
        /// <paramref name="destFileName"/> already exists and
        /// <paramref name="overwrite"/> is <see langword="false"/>.
        /// -or-
        /// An I/O error has occurred.
        /// </exception>
        public static void Copy(string sourceFileName, string destFileName, bool overwrite)
        {
            ThrowIfParamPathIsNullOrEmpty(sourceFileName, nameof(sourceFileName));
            ThrowIfParamPathIsNullOrEmpty(destFileName, nameof(destFileName));

            if (Directory.Exists(destFileName))
            {
                throw new ArgumentException($"The target file '{destFileName}' is a directory, not a file.", nameof(destFileName));
            }

            using var sourceFileStream = Open(sourceFileName, SystemFileAccess.Read);

            if (!overwrite && Exists(destFileName))
            {
                throw new IOException($"The file '{destFileName}' already exists.");
            }

            using var destFileStream = Open(destFileName, SystemFileAccess.Write);
            sourceFileStream.CopyTo(destFileStream);
        }

        /// <summary>
        /// Creates or overwrites a file in the specified path.
        /// </summary>
        /// <param name="path">The path and name of the file to create.</param>
        /// <returns>
        /// A <see cref="FileStream"/> that provides read/write access to
        /// the file specified in <paramref name="path"/>.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while creating the file.
        /// </exception>
        public static FileStream Create(string path)
        {
            return Open(path, SystemFileAccess.ReadWrite);
        }

        /// <summary>
        /// Deletes the specified file.
        /// </summary>
        /// <param name="path">The name of the file to be deleted.</param>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while deleting the file.
        /// </exception>
        public static void Delete(string path)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            DirAccess.RemoveAbsolute(path).ThrowIfError();
        }

        /// <summary>
        /// Determines whether the specified file exists.
        /// </summary>
        /// <param name="path">The file to check.</param>
        /// <returns>
        /// <see langword="true"/> if the path refers to an existing file;
        /// otherwise, <see langword="false"/>.
        /// </returns>
        public static bool Exists([NotNullWhen(true)] string? path)
        {
            if (string.IsNullOrEmpty(path))
            {
                return false;
            }

            return FileAccess.FileExists(path);
        }

        private static FileAccess.ModeFlags FileAccessToModeFlags(SystemFileAccess access)
        {
            return access switch
            {
                SystemFileAccess.Read => FileAccess.ModeFlags.Read,
                SystemFileAccess.Write => FileAccess.ModeFlags.Write,
                SystemFileAccess.ReadWrite => FileAccess.ModeFlags.ReadWrite, // Assume FileMode.Create
                _ => throw new ArgumentOutOfRangeException(nameof(access), access, message: null),
            };
        }

        /// <summary>
        /// Opens a <see cref="FileStream"/> on the specified path.
        /// </summary>
        /// <param name="path">The file to open.</param>
        /// <returns>
        /// A <see cref="FileStream"/> that provides access to the specified file.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static FileStream Open(string path)
        {
            return Open(path, SystemFileAccess.ReadWrite);
        }

        /// <summary>
        /// Opens a <see cref="FileStream"/> on the specified path, with the
        /// specified access.
        /// </summary>
        /// <param name="path">The file to open.</param>
        /// <param name="access">
        /// A <see cref="SystemFileAccess"/> value that specifies the operations
        /// that can be performed on the file.
        /// </param>
        /// <returns>
        /// A <see cref="FileStream"/> that provides access to the specified file,
        /// with the specified access.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static FileStream Open(string path, SystemFileAccess access)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            var file = FileAccess.Open(path, FileAccessToModeFlags(access));

            FileAccess.GetOpenError().ThrowIfError();

            return new FileStream(file, access);
        }

        /// <summary>
        /// Opens a compressed <see cref="FileStream"/> on the specified path, with the
        /// specified access.
        /// </summary>
        /// <param name="path">The file to open.</param>
        /// <param name="access">
        /// A <see cref="SystemFileAccess"/> value that specifies the operations
        /// that can be performed on the file.
        /// </param>
        /// <param name="mode">
        /// A <see cref="FileAccess.CompressionMode"/> value that specifies the
        /// compression method to use when compressing/decompressing the file.
        /// </param>
        /// <returns>
        /// A <see cref="FileStream"/> that provides access to the specified file,
        /// with the specified access.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static FileStream OpenCompressed(string path, SystemFileAccess access, FileAccess.CompressionMode mode = FileAccess.CompressionMode.Fastlz)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            var file = FileAccess.OpenCompressed(path, FileAccessToModeFlags(access), mode);

            FileAccess.GetOpenError().ThrowIfError();

            return new FileStream(file, access);
        }

        /// <summary>
        /// Opens an encrypted <see cref="FileStream"/> on the specified path, with the
        /// specified access.
        /// </summary>
        /// <param name="path">The file to open.</param>
        /// <param name="access">
        /// A <see cref="SystemFileAccess"/> value that specifies the operations
        /// that can be performed on the file.
        /// </param>
        /// <param name="key">
        /// The cryptographic key to use when encrypting/decrypting the file.
        /// </param>
        /// <returns>
        /// A <see cref="FileStream"/> that provides access to the specified file,
        /// with the specified access.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="key"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// -or-
        /// <paramref name="key"/> is empty.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static FileStream OpenEncrypted(string path, SystemFileAccess access, byte[] key)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (key == null)
            {
                throw new ArgumentNullException(nameof(key), "Key cannot be null.");
            }
            if (key.Length == 0)
            {
                throw new ArgumentException("Key cannot be empty.", nameof(key));
            }

            var file = FileAccess.OpenEncrypted(path, FileAccessToModeFlags(access), key);

            FileAccess.GetOpenError().ThrowIfError();

            return new FileStream(file, access);
        }

        /// <summary>
        /// Opens an encrypted <see cref="FileStream"/> on the specified path, with the
        /// specified access.
        /// </summary>
        /// <param name="path">The file to open.</param>
        /// <param name="access">
        /// A <see cref="SystemFileAccess"/> value that specifies the operations
        /// that can be performed on the file.
        /// </param>
        /// <param name="pass">
        /// The passphrase to use when encrypting/decrypting the file.
        /// </param>
        /// <returns>
        /// A <see cref="FileStream"/> that provides access to the specified file,
        /// with the specified access.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="pass"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// -or-
        /// <paramref name="pass"/> is a zero-length string.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static FileStream OpenEncryptedWithPass(string path, SystemFileAccess access, string pass)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (pass == null)
            {
                throw new ArgumentNullException(nameof(pass), "Password cannot be null.");
            }
            if (pass.Length == 0)
            {
                throw new ArgumentException("Password cannot be empty.", nameof(pass));
            }

            var file = FileAccess.OpenEncryptedWithPass(path, FileAccessToModeFlags(access), pass);

            FileAccess.GetOpenError().ThrowIfError();

            return new FileStream(file, access);
        }

        /// <summary>
        /// Returns the creation date and time of the specified file or directory.
        /// </summary>
        /// <param name="path">
        /// The file or directory for which to obtain creation date and time information.
        /// </param>
        /// <returns>
        /// A <see cref="DateTime"/> structure set to the creation date and time
        /// for the specified file or directory. This value is expressed in local time.
        /// </returns>
        public static DateTime GetCreationTime(string path)
        {
            var dateTime = DateTimeOffset.FromUnixTimeSeconds((long)FileAccess.GetModifiedTime(path));
            return dateTime.LocalDateTime;
        }

        /// <summary>
        /// Returns the creation date and time, in Coordinated Universal Time (UTC),
        /// of the specified file or directory.
        /// </summary>
        /// <param name="path">
        /// The file or directory for which to obtain creation date and time information.
        /// </param>
        /// <returns>
        /// A <see cref="DateTime"/> structure set to the creation date and time
        /// for the specified file or directory. This value is expressed in UTC time.
        /// </returns>
        public static DateTime GetCreationTimeUtc(string path)
        {
            var dateTime = DateTimeOffset.FromUnixTimeSeconds((long)FileAccess.GetModifiedTime(path));
            return dateTime.UtcDateTime;
        }

        /// <summary>
        /// Opens an existing file for reading.
        /// </summary>
        /// <param name="path">The file to be opened for reading.</param>
        /// <returns>
        /// A read-only <see cref="FileStream"/> on the specified path.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static FileStream OpenRead(string path)
        {
            return Open(path, SystemFileAccess.Read);
        }

        /// <summary>
        /// Opens an existing file or creates a new file for writing.
        /// </summary>
        /// <param name="path">The file to be opened for writing.</param>
        /// <returns>
        /// A <see cref="FileStream"/> object on the specified path with
        /// <see cref="SystemFileAccess.Write"/> access.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static FileStream OpenWrite(string path)
        {
            return Open(path, SystemFileAccess.Write);
        }

        /// <summary>
        /// Opens a text file, reads all the text in the file into a string,
        /// and then closes the file.
        /// </summary>
        /// <param name="path">The file to open for reading.</param>
        /// <returns>A string containing all the text in the file.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static string ReadAllText(string path)
        {
            return ReadAllText(path, Encoding.UTF8);
        }

        /// <summary>
        /// Opens a file, reads all text in the file with the specified encoding,
        /// and then closes the file.
        /// </summary>
        /// <param name="path">The file to open for reading.</param>
        /// <param name="encoding">
        /// The encoding applied to the contents of the file.
        /// </param>
        /// <returns>A string containing all the text in the file.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="encoding"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static string ReadAllText(string path, Encoding encoding)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (encoding == null)
            {
                throw new ArgumentNullException(nameof(encoding));
            }

            using var stream = new StreamReader(Open(path, SystemFileAccess.Read), encoding, detectEncodingFromByteOrderMarks: true);
            return stream.ReadToEnd();
        }

        /// <summary>
        /// Creates a new file, write the contents to the file, and then closes
        /// the file. If the target file already exists, it is overwritten.
        /// </summary>
        /// <param name="path">The file to write to.</param>
        /// <param name="contents">The string to write to the file.</param>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static void WriteAllText(string path, string? contents)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            using var stream = new StreamWriter(Open(path, SystemFileAccess.Write));
            stream.Write(contents);
        }

        /// <summary>
        /// Creates a new file, writes the specified string to the file using
        /// the specified encoding, and then closes the file. If the target file
        /// already exists, it is overwritten.
        /// </summary>
        /// <param name="path">The file to write to.</param>
        /// <param name="contents">The string to write to the file.</param>
        /// <param name="encoding">The encoding to apply to the string.</param>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="encoding"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static void WriteAllText(string path, string? contents, Encoding encoding)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (encoding == null)
            {
                throw new ArgumentNullException(nameof(encoding));
            }

            using var stream = new StreamWriter(Open(path, SystemFileAccess.Write), encoding);
            stream.Write(contents);
        }

        /// <summary>
        /// Opens a binary file, reads the contents of the file into a byte array,
        /// and then closes the file.
        /// </summary>
        /// <param name="path">The file to open for reading.</param>
        /// <returns>A byte array containing the contents of the file.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static byte[] ReadAllBytes(string path)
        {
            using var stream = Open(path, SystemFileAccess.Read);

            long length = stream.Length;

            if (length > int.MaxValue)
            {
                throw new IOException("The file is too long. Only files less than 2 GBs in size are supported.");
            }

            int offset = 0;
            var bytes = new byte[length];
            int count = (int)length;
            while (count > 0)
            {
                int bytesRead = stream.Read(bytes, offset, count);

                if (bytesRead == 0)
                {
                    throw new EndOfStreamException();
                }

                offset += bytesRead;
                count -= bytesRead;
            }

            return bytes;
        }

        /// <summary>
        /// Creates a new file, writes the specified byte array to the file,
        /// and then closes the file. If the target file already exists,
        /// it is overwritten.
        /// </summary>
        /// <param name="path">The file to write to.</param>
        /// <param name="bytes">The bytes to write to the file.</param>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="bytes"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static void WriteAllBytes(string path, byte[] bytes)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (bytes == null)
            {
                throw new ArgumentNullException(nameof(bytes));
            }

            using var stream = Open(path, SystemFileAccess.Write);
            stream.Write(bytes, offset: 0, count: bytes.Length);
        }

        /// <summary>
        /// Opens a text file, reads all lines of the file, and then closes the file.
        /// </summary>
        /// <param name="path">The file to open for reading.</param>
        /// <returns>A string array containing all lines of the file.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static string[] ReadAllLines(string path)
        {
            return ReadAllLines(path, Encoding.UTF8);
        }

        /// <summary>
        /// Opens a file, reads all lines of the file with the specified encoding,
        /// and then closes the file.
        /// </summary>
        /// <param name="path">The file to open for reading.</param>
        /// <param name="encoding">The encoding applied to the contents of the file.</param>
        /// <returns>A string array containing all lines of the file.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="encoding"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static string[] ReadAllLines(string path, Encoding encoding)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (encoding == null)
            {
                throw new ArgumentNullException(nameof(encoding));
            }

            using var stream = new StreamReader(Open(path, SystemFileAccess.Read), encoding);

            string? line;
            var lines = new List<string>();
            while ((line = stream.ReadLine()) != null)
            {
                lines.Add(line);
            }

            return lines.ToArray();
        }

        /// <summary>
        /// Reads the lines of a file.
        /// </summary>
        /// <param name="path">The file to read.</param>
        /// <returns>All the lines of the file.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static IEnumerable<string> ReadLines(string path)
        {
            return ReadLines(path, Encoding.UTF8);
        }

        /// <summary>
        /// Read the lines of a file that has a specified encoding.
        /// </summary>
        /// <param name="path">The file to read.</param>
        /// <param name="encoding">The encoding that is applied to the contents of the file.</param>
        /// <returns>All the lines of the file.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="encoding"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static IEnumerable<string> ReadLines(string path, Encoding encoding)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (encoding == null)
            {
                throw new ArgumentNullException(nameof(encoding));
            }

            using var stream = new StreamReader(Open(path, SystemFileAccess.Read), encoding);

            string? line;
            while ((line = stream.ReadLine()) != null)
            {
                yield return line;
            }
        }

        /// <summary>
        /// Creates a new file, writes a collection of strings to the file,
        /// and then closes the file.
        /// </summary>
        /// <param name="path">The file to write to.</param>
        /// <param name="contents">The lines to write to the file.</param>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="contents"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static void WriteAllLines(string path, IEnumerable<string> contents)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (contents == null)
            {
                throw new ArgumentNullException(nameof(contents));
            }

            using var stream = new StreamWriter(Open(path, SystemFileAccess.Write));

            foreach (string line in contents)
            {
                stream.WriteLine(line);
            }
        }

        /// <summary>
        /// Creates a new file by using the specified encoding, writes a
        /// collection of strings to the file, and then closes the file.
        /// </summary>
        /// <param name="path">The file to write to.</param>
        /// <param name="contents">The lines to write to the file.</param>
        /// <param name="encoding">The character encoding to use.</param>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/>, <paramref name="contents"/> or
        /// <paramref name="encoding"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static void WriteAllLines(string path, IEnumerable<string> contents, Encoding encoding)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (contents == null)
            {
                throw new ArgumentNullException(nameof(contents));
            }
            if (encoding == null)
            {
                throw new ArgumentNullException(nameof(encoding));
            }

            using var stream = new StreamWriter(Open(path, SystemFileAccess.Write), encoding);

            foreach (string line in contents)
            {
                stream.WriteLine(line);
            }
        }

        /// <summary>
        /// Moves a specified file to a new location, providing the option to
        /// specify a new file name.
        /// </summary>
        /// <param name="sourceFileName">The name of the file to move.</param>
        /// <param name="destFileName">The new path and name for the file.</param>
        /// <exception cref="ArgumentException">
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// is a zero-length string, contains only white space, or contains one
        /// or more invalid characters.
        /// -or-
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// specifies a directory.
        /// </exception>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="FileNotFoundException">
        /// <paramref name="sourceFileName"/> does not exist or could not be found.
        /// </exception>
        /// <exception cref="IOException">
        /// A file with the same name and location specified by
        /// <paramref name="destFileName"/> already exists.
        /// -or-
        /// An I/O error has occurred.
        /// </exception>
        public static void Move(string sourceFileName, string destFileName)
        {
            Move(sourceFileName, destFileName, overwrite: false);
        }

        /// <summary>
        /// Moves a specified file to a new location, providing the options to
        /// specify a new file name and to overwrite the destination file if
        /// it already exists.
        /// </summary>
        /// <param name="sourceFileName">The name of the file to move.</param>
        /// <param name="destFileName">The new path and name for the file.</param>
        /// <param name="overwrite">
        /// <see langword="true"/> to overwrite the destination file if it
        /// already exists; <see langword="false"/> otherwise.
        /// </param>
        /// <exception cref="ArgumentException">
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// is a zero-length string, contains only white space, or contains one
        /// or more invalid characters.
        /// -or-
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// specifies a directory.
        /// </exception>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="sourceFileName"/> or <paramref name="destFileName"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="FileNotFoundException">
        /// <paramref name="sourceFileName"/> does not exist or could not be found.
        /// </exception>
        /// <exception cref="IOException">
        /// A file with the same name and location specified by
        /// <paramref name="destFileName"/> already exists and
        /// <paramref name="overwrite"/> is <see langword="false"/>.
        /// -or-
        /// An I/O error has occurred.
        /// </exception>
        public static void Move(string sourceFileName, string destFileName, bool overwrite)
        {
            ThrowIfParamPathIsNullOrEmpty(sourceFileName, nameof(sourceFileName));
            ThrowIfParamPathIsNullOrEmpty(destFileName, nameof(destFileName));

            if (!Exists(sourceFileName))
            {
                throw new FileNotFoundException($"The source file '{sourceFileName}' could not be found", sourceFileName);
            }

            // Don't use DirAccess.Rename because it can't move files across multiple mount points.
            Copy(sourceFileName, destFileName, overwrite);
            Delete(sourceFileName);
        }

        // TODO: Try to improve async implementations. Currently blocked due to limitations with the Godot IO API.

        /// <summary>
        /// Asynchronously opens a text file, reads all the text in the file,
        /// and then closes the file.
        /// </summary>
        /// <param name="path">The file to open for reading.</param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>
        /// A task that represents the asynchronous read operation, which wraps
        /// the string containing all text in the file.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static Task<string> ReadAllTextAsync(string path, CancellationToken cancellationToken = default)
        {
            return ReadAllTextAsync(path, Encoding.UTF8, cancellationToken);
        }

        /// <summary>
        /// Asynchronously opens a text file, reads all text in the file with
        /// the specified encoding, and then closes the file.
        /// </summary>
        /// <param name="path">The file to open for reading.</param>
        /// <param name="encoding">
        /// The encoding applied to the contents of the file.
        /// </param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>
        /// A task that represents the asynchronous read operation, which wraps
        /// the string containing all text in the file.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="encoding"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static Task<string> ReadAllTextAsync(string path, Encoding encoding, CancellationToken cancellationToken = default)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (encoding == null)
            {
                throw new ArgumentNullException(nameof(encoding));
            }

            if (cancellationToken.IsCancellationRequested)
            {
                return Task.FromCanceled<string>(cancellationToken);
            }

            return ReadAllTextAsyncCore(path, encoding, cancellationToken);
        }

        private static async Task<string> ReadAllTextAsyncCore(string path, Encoding encoding, CancellationToken cancellationToken)
        {
            char[]? buffer = null;

            using var stream = new StreamReader(Open(path, SystemFileAccess.Read), encoding);

            try
            {
                cancellationToken.ThrowIfCancellationRequested();
                buffer = ArrayPool<char>.Shared.Rent(stream.CurrentEncoding.GetMaxCharCount(BufferSize));

                var builder = new StringBuilder();

                while (true)
                {
                    int read = await stream.ReadAsync(new Memory<char>(buffer), cancellationToken).ConfigureAwait(false);

                    if (read == 0)
                    {
                        return builder.ToString();
                    }

                    builder.Append(buffer, 0, read);
                }
            }
            finally
            {
                if (buffer != null)
                {
                    ArrayPool<char>.Shared.Return(buffer);
                }
            }
        }

        /// <summary>
        /// Asynchronously creates a new file, writes the specified string to
        /// the file, and then closes the file. If the target file already exists,
        /// it is overwritten.
        /// </summary>
        /// <param name="path">The file to write to.</param>
        /// <param name="contents">The string to write to the file.</param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>A task that represents the asynchronous write operation.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static Task WriteAllTextAsync(string path, string? contents, CancellationToken cancellationToken = default)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (cancellationToken.IsCancellationRequested)
            {
                return Task.FromCanceled(cancellationToken);
            }

            if (string.IsNullOrEmpty(contents))
            {
                return Task.CompletedTask;
            }

            using var writer = new StreamWriter(Open(path, SystemFileAccess.Write));
            return WriteAllTextAsyncCore(writer, contents, cancellationToken);
        }

        /// <summary>
        /// Asynchronously creates a new file, writes the specified string to
        /// the file using the specified encoding, and then closes the file.
        /// If the target file already exists, it is overwritten.
        /// </summary>
        /// <param name="path">The file to write to.</param>
        /// <param name="contents">The string to write to the file.</param>
        /// <param name="encoding">The encoding to apply to the string.</param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>A task that represents the asynchronous write operation.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="encoding"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static Task WriteAllTextAsync(string path, string? contents, Encoding encoding, CancellationToken cancellationToken = default)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (encoding == null)
            {
                throw new ArgumentNullException(nameof(encoding));
            }

            if (cancellationToken.IsCancellationRequested)
            {
                return Task.FromCanceled(cancellationToken);
            }

            if (string.IsNullOrEmpty(contents))
            {
                return Task.CompletedTask;
            }

            using var writer = new StreamWriter(Open(path, SystemFileAccess.Write), encoding);
            return WriteAllTextAsyncCore(writer, contents, cancellationToken);
        }

        /// <summary>
        /// Asynchronously opens a binary file, reads the contents of the file
        /// into a byte array, and then closes the file.
        /// </summary>
        /// <param name="path">The file to open for reading.</param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>
        /// A task that represents the asynchronous read operation, which wraps
        /// the byte array containing the contents of the file.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static async Task<byte[]> ReadAllBytesAsync(string path, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();

            using var stream = Open(path, SystemFileAccess.Read);

            long lengthLong = stream.Length;
            if (lengthLong > int.MaxValue)
            {
                throw new IOException("The file is too long. Only files less than 2 GBs in size are supported.");
            }

            int length = (int)lengthLong;

            int index = 0;
            var bytes = new byte[length];
            do
            {
                var buffer = new Memory<byte>(bytes, index, length - index);
                int bytesRead = await stream.ReadAsync(buffer, cancellationToken).ConfigureAwait(false);

                if (bytesRead == 0)
                {
                    throw new EndOfStreamException();
                }

                index += bytesRead;
            } while (index < length);

            return bytes;
        }

        /// <summary>
        /// Asynchronously creates a new file, writes the specified byte array
        /// to the file, and then closes the file. If the target file already
        /// exists, it is overwritten.
        /// </summary>
        /// <param name="path">The file to write to.</param>
        /// <param name="bytes">The bytes to write to the file.</param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>A task that represents the asynchronous write operation.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="bytes"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static Task WriteAllBytesAsync(string path, byte[] bytes, CancellationToken cancellationToken = default)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (bytes == null)
            {
                throw new ArgumentNullException(nameof(bytes));
            }

            if (cancellationToken.IsCancellationRequested)
            {
                return Task.FromCanceled(cancellationToken);
            }

            return WriteAllBytesAsyncCore(path, bytes, cancellationToken);
        }

        private static async Task WriteAllBytesAsyncCore(string path, byte[] bytes, CancellationToken cancellationToken)
        {
            using var stream = Open(path, SystemFileAccess.Write);

            await stream.WriteAsync(new ReadOnlyMemory<byte>(bytes), cancellationToken).ConfigureAwait(false);
            await stream.FlushAsync(cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Asynchronously opens a text file, reads all lines of the file,
        /// and then closes the file.
        /// </summary>
        /// <param name="path">The file to open for reading.</param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>
        /// A task that represents the asynchronous read operation, which wraps
        /// the string array containing all lines of the file.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static Task<string[]> ReadAllLinesAsync(string path, CancellationToken cancellationToken = default)
        {
            return ReadAllLinesAsync(path, Encoding.UTF8, cancellationToken);
        }

        /// <summary>
        /// Asynchronously opens a text file, reads all lines of the file with
        /// the specified encoding, and then closes the file.
        /// </summary>
        /// <param name="path">The file to open for reading.</param>
        /// <param name="encoding">
        /// The encoding that is applied to the contents of the file.
        /// </param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>
        /// A task that represents the asynchronous read operation, which wraps
        /// the string array containing all lines of the file.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="encoding"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static Task<string[]> ReadAllLinesAsync(string path, Encoding encoding, CancellationToken cancellationToken = default)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (encoding == null)
            {
                throw new ArgumentNullException(nameof(encoding));
            }

            if (cancellationToken.IsCancellationRequested)
            {
                return Task.FromCanceled<string[]>(cancellationToken);
            }

            return ReadAllLinesAsyncCore(path, encoding, cancellationToken);
        }

        private static async Task<string[]> ReadAllLinesAsyncCore(string path, Encoding encoding, CancellationToken cancellationToken)
        {
            using var stream = new StreamReader(Open(path, SystemFileAccess.Read), encoding);

            cancellationToken.ThrowIfCancellationRequested();

            var lines = new List<string>();
            string? line;
            while ((line = await stream.ReadLineAsync().ConfigureAwait(false)) != null)
            {
                lines.Add(line);
                cancellationToken.ThrowIfCancellationRequested();
            }

            return lines.ToArray();
        }

        /// <summary>
        /// Asynchronously reads the lines of a file.
        /// </summary>
        /// <param name="path">The file to read.</param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns></returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static IAsyncEnumerable<string> ReadLinesAsync(string path, CancellationToken cancellationToken = default)
        {
            return ReadLinesAsync(path, Encoding.UTF8, cancellationToken);
        }

        /// <summary>
        /// Asynchronously reads the lines of a file that has a specified encoding.
        /// </summary>
        /// <param name="path">The file to read.</param>
        /// <param name="encoding">
        /// The encoding that is applied to the contents of the file.
        /// </param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns></returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="encoding"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static IAsyncEnumerable<string> ReadLinesAsync(string path, Encoding encoding, CancellationToken cancellationToken = default)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (encoding == null)
            {
                throw new ArgumentNullException(nameof(encoding));
            }

            return ReadLinesAsyncCore(path, encoding, cancellationToken);
        }

        private static async IAsyncEnumerable<string> ReadLinesAsyncCore(string path, Encoding encoding, [EnumeratorCancellation] CancellationToken cancellationToken)
        {
            using var stream = new StreamReader(Open(path, SystemFileAccess.Read), encoding);

            cancellationToken.ThrowIfCancellationRequested();

            string? line;
            while ((line = await stream.ReadLineAsync().ConfigureAwait(false)) != null)
            {
                yield return line;
                cancellationToken.ThrowIfCancellationRequested();
            }
        }

        /// <summary>
        /// Asynchronously creates a new file, writes the specified lines to the
        /// file, and then closes the file.
        /// </summary>
        /// <param name="path">The file to write to.</param>
        /// <param name="contents">The lines to write to the file.</param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>A task that represents the asynchronous write operation.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> or <paramref name="contents"/>
        /// is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static Task WriteAllLinesAsync(string path, IEnumerable<string> contents, CancellationToken cancellationToken = default)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (contents == null)
            {
                throw new ArgumentNullException(nameof(contents));
            }

            if (cancellationToken.IsCancellationRequested)
            {
                return Task.FromCanceled(cancellationToken);
            }

            using var writer = new StreamWriter(Open(path, SystemFileAccess.Write));
            return WriteAllLinesAsyncCore(writer, contents, cancellationToken);
        }

        /// <summary>
        /// Asynchronously creates a new file, write the specified lines to the
        /// file by using the specified encoding, and then closes the file.
        /// </summary>
        /// <param name="path">The file to write to.</param>
        /// <param name="contents">The lines to write to the file.</param>
        /// <param name="encoding">The character encoding to use.</param>
        /// <param name="cancellationToken">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>A task that represents the asynchronous write operation.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/>, <paramref name="contents"/> or
        /// <paramref name="encoding"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static Task WriteAllLinesAsync(string path, IEnumerable<string> contents, Encoding encoding, CancellationToken cancellationToken = default)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (contents == null)
            {
                throw new ArgumentNullException(nameof(contents));
            }
            if (encoding == null)
            {
                throw new ArgumentNullException(nameof(encoding));
            }

            if (cancellationToken.IsCancellationRequested)
            {
                return Task.FromCanceled(cancellationToken);
            }

            using var writer = new StreamWriter(Open(path, SystemFileAccess.Write), encoding);
            return WriteAllLinesAsyncCore(writer, contents, cancellationToken);
        }

        private static async Task WriteAllLinesAsyncCore(TextWriter writer, IEnumerable<string> contents, CancellationToken cancellationToken)
        {
            foreach (string line in contents)
            {
                cancellationToken.ThrowIfCancellationRequested();
                await writer.WriteLineAsync(line).ConfigureAwait(false);
            }

            cancellationToken.ThrowIfCancellationRequested();
            await writer.FlushAsync().ConfigureAwait(false);
        }

        private static async Task WriteAllTextAsyncCore(StreamWriter stream, string contents, CancellationToken cancellationToken)
        {
            char[]? buffer = null;

            try
            {
                buffer = ArrayPool<char>.Shared.Rent(BufferSize);
                int count = contents.Length;
                int index = 0;
                while (index < count)
                {
                    int length = Math.Min(BufferSize, count - index);
                    contents.CopyTo(index, buffer, 0, length);
                    await stream.WriteAsync(new ReadOnlyMemory<char>(buffer, 0, length), cancellationToken).ConfigureAwait(false);
                    index += length;
                }

                cancellationToken.ThrowIfCancellationRequested();
                await stream.FlushAsync().ConfigureAwait(false);
            }
            finally
            {
                if (buffer != null)
                {
                    ArrayPool<char>.Shared.Return(buffer);
                }
            }
        }

        private static void ThrowIfParamPathIsNullOrEmpty(string path, string paramName)
        {
            if (path == null)
            {
                throw new ArgumentNullException(paramName, "File name cannot be null.");
            }

            if (string.IsNullOrWhiteSpace(path))
            {
                throw new ArgumentException("Empty file name is not legal.", paramName);
            }
        }
    }
}
