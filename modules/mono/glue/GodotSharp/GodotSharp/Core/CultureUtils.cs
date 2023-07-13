using System.Globalization;
using System.Runtime.InteropServices;

namespace Godot
{
    internal static class CultureUtils
    {
        /// <summary>
        /// Updates the current culture to the culture that corresponds
        /// to the given Godot locale.
        /// </summary>
        /// <param name="locale">Godot locale.</param>
        internal static unsafe void UpdateCurrentCulture(string locale)
        {
            // C# uses underscore separators for culture names, but Godot locales use dashes.
            locale = locale.Replace('_', '-');
            try
            {
                CultureInfo.CurrentUICulture = CultureInfo.CreateSpecificCulture(locale);
            }
            catch (CultureNotFoundException)
            {
                GD.PushWarning($"Culture '{locale}' not found, culture remains unchanged.");
            }
        }
    }
}
