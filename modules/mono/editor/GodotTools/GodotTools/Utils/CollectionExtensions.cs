using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;

#nullable enable

namespace GodotTools.Utils
{
    public static class CollectionExtensions
    {
        [return: NotNullIfNotNull(nameof(orElse))]
        public static TResult? SelectFirstNotNull<TSource, TResult>(this IEnumerable<TSource> enumerable, Func<TSource, TResult?> predicate, TResult? orElse = null)
            where TResult : class
        {
            foreach (TSource elem in enumerable)
            {
                TResult? result = predicate(elem);
                if (result != null)
                    return result;
            }

            return orElse;
        }

        public static IEnumerable<string> EnumerateLines(this TextReader textReader)
        {
            string? line;
            while ((line = textReader.ReadLine()) != null)
                yield return line;
        }
    }
}
