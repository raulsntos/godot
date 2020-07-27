using System;
using System.Diagnostics.CodeAnalysis;
using System.IO;

#nullable enable

namespace Godot
{
    internal static class ErrorExtensions
    {
        /// <summary>
        /// Converts an error into an exception.
        /// </summary>
        /// <param name="error">The <see cref="Error"/> to convert.</param>
        /// <param name="exception">
        /// The <see cref="Exception"/> equivalent to <paramref name="error"/>
        /// or <see langword="null"/> if the error is <see cref="Error.Ok"/>.
        /// </param>
        /// <returns>
        /// <see langword="true"/> if <paramref name="error"/> is not <see cref="Error.Ok"/>.
        /// </returns>
        /// <exception cref="ArgumentException">
        /// <paramref name="error"/> is <see cref="Error.InvalidParameter"/>.
        /// </exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <paramref name="error"/> is <see cref="Error.ParameterRangeError"/>.
        /// </exception>
        /// <exception cref="FileNotFoundException">
        /// <paramref name="error"/> is <see cref="Error.FileBadDrive"/>,
        /// <see cref="Error.FileBadPath"/> or <see cref="Error.FileNotFound"/>.
        /// </exception>
        /// <exception cref="IOException">
        /// <paramref name="error"/> is one of the errors related to I/O, such as
        /// <see cref="Error.FileCantOpen"/>.
        /// </exception>
        /// <exception cref="InvalidDataException">
        /// <paramref name="error"/> is <see cref="Error.InvalidData"/>.
        /// </exception>
        /// <exception cref="OutOfMemoryException">
        /// <paramref name="error"/> is <see cref="Error.OutOfMemory"/>.
        /// </exception>
        /// <exception cref="TimeoutException">
        /// <paramref name="error"/> is <see cref="Error.Timeout"/>.
        /// </exception>
        /// <exception cref="UnauthorizedAccessException">
        /// <paramref name="error"/> is <see cref="Error.FileNoPermission"/>
        /// or <see cref="Error.Unauthorized"/>.
        /// </exception>
        /// <exception cref="Exception">
        /// <paramref name="error"/> is an unknown error or a more specific
        /// exception could not be determined.
        /// </exception>
        public static bool TryGetException(this Error error, [NotNullWhen(true)] out Exception? exception)
        {
            if (error == Error.Ok)
            {
                exception = null;
                return false;
            }

            string msg = $"Method returned with Godot error: '{error}'.";

            exception = error switch
            {
                Error.InvalidParameter => new ArgumentException(message: msg, paramName: null),
                Error.ParameterRangeError => new ArgumentOutOfRangeException(paramName: null, message: msg),
                Error.FileBadDrive or
                Error.FileBadPath or
                Error.FileNotFound => new FileNotFoundException(msg),
                Error.AlreadyInUse or
                Error.CantAcquireResource or
                Error.CantOpen or
                Error.CantCreate or
                Error.FileAlreadyInUse or
                Error.FileCantOpen or
                Error.FileCantRead or
                Error.FileCantWrite or
                Error.FileEof or
                Error.Locked => new IOException(msg),
                Error.InvalidData => new InvalidDataException(msg),
                // TODO: Exceptions of type OutOfMemoryException is reserved by the runtime
                Error.OutOfMemory => new OutOfMemoryException(msg),
                Error.Timeout => new TimeoutException(msg),
                Error.FileNoPermission or
                Error.Unauthorized => new UnauthorizedAccessException(msg),
                _ => new InvalidOperationException(msg),
            };
            return true;
        }
    }

    public static void ThrowIfError(this Error error)
    {
        if (error.TryGetException(out var exception))
        {
            throw exception;
        }
    }
}
