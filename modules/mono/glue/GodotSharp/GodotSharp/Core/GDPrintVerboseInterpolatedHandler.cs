using System;
using System.Collections.Generic;
using System.Text;
using Godot.NativeInterop;

namespace Godot
{
    [InterpolatedStringHandler]
    public ref struct GDPrintVerboseInterpolatedStringHandler
    {
        /// <summary>
        /// The handler used to perform the formatting.
        /// </summary>
        private StringBuilder.AppendInterpolatedStringHandler _sbHandler;

        /// <summary>
        /// The underlying <see cref="StringBuilder"/> instance used by <see cref="_sbHandler"/>, if any.
        /// </summary>
        private StringBuilder? _sb;

        public GDPrintInterpolatedStringHandler(int literalLength, int formattedCount, out bool isEnabled)
        {
            if (OS.IsStdoutVerbose())
            {
                _sbHandler = new StringBuilder.AppendInterpolatedStringHandler(literalLength, formattedCount, _sb);
                isEnabled = true;
            }
            else
            {
                _sbHandler = default;
                isEnabled = false;
            }
        }

        /// <summary>
        /// Extracts the built string from the handler.
        /// </summary>
        internal string ToStringAndClear()
        {
            string str = _sb?.ToString() ?? string.Empty;
            this = default;
            return str;
        }

        /// <inheritdoc cref="StringBuilder.AppendInterpolatedStringHandler.AppendLiteral(string)"/>
        public void AppendLiteral(string s)
        {
            _sbHandler.AppendLiteral(s);
        }

        /// <inheritdoc cref="StringBuilder.AppendInterpolatedStringHandler.AppendFormatted{T}(T)"/>
        public void AppendFormatted<T>(T value)
        {
            _sbHandler.AppendFormatted(value);
        }

        /// <inheritdoc cref="StringBuilder.AppendInterpolatedStringHandler.AppendFormatted{T}(T, string?)"/>
        public void AppendFormatted<T>(T value, string? format)
        {
            _sbHandler.AppendFormatted(value, format);
        }

        /// <inheritdoc cref="StringBuilder.AppendInterpolatedStringHandler.AppendFormatted{T}(T, int)"/>
        public void AppendFormatted<T>(T value, int alignment)
        {
            _sbHandler.AppendFormatted(value, alignment);
        }

        /// <inheritdoc cref="StringBuilder.AppendInterpolatedStringHandler.AppendFormatted{T}(T, int, string?)"/>
        public void AppendFormatted<T>(T value, int alignment, string? format)
        {
            _sbHandler.AppendFormatted(value, alignment, format);
        }

        /// <inheritdoc cref="StringBuilder.AppendInterpolatedStringHandler.AppendFormatted(ReadOnlySpan{char})"/>
        public void AppendFormatted(ReadOnlySpan<char> value)
        {
            _sbHandler.AppendFormatted(value);
        }

        /// <inheritdoc cref="StringBuilder.AppendInterpolatedStringHandler.AppendFormatted(ReadOnlySpan{char}, int, string?)"/>
        public void AppendFormatted(ReadOnlySpan<char> value, int alignment = 0, string? format = null)
        {
            _sbHandler.AppendFormatted(value, alignment, format);
        }

        /// <inheritdoc cref="StringBuilder.AppendInterpolatedStringHandler.AppendFormatted(string?)"/>
        public void AppendFormatted(string? value)
        {
            _sbHandler.AppendFormatted(value);
        }

        /// <inheritdoc cref="StringBuilder.AppendInterpolatedStringHandler.AppendFormatted(string?, int, string?)"/>
        public void AppendFormatted(string? value, int alignment = 0, string? format = null)
        {
            _sbHandler.AppendFormatted(value, alignment, format);
        }

        /// <inheritdoc cref="StringBuilder.AppendInterpolatedStringHandler.AppendFormatted(object?, int, string?)"/>
        public void AppendFormatted(object? value, int alignment = 0, string? format = null)
        {
            _sbHandler.AppendFormatted(value, alignment, format);
        }
    }
}
