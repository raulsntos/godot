using System;

namespace Godot
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public class ExportAttribute : Attribute
    {
        private PropertyHint hint;
        private string hintString;
        private PropertyUsageFlags usage;

        public ExportAttribute(PropertyHint hint = PropertyHint.None, string hintString = "", PropertyUsageFlags usage = PropertyUsageFlags.Default)
        {
            this.hint = hint;
            this.hintString = hintString;
            this.usage = usage;
        }
    }
}
