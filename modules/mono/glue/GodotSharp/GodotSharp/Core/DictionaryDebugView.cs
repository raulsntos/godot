using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace Godot
{
    internal sealed class DictionaryDebugView
    {
        private readonly Godot.Collections.Dictionary _dictionary;

        public DictionaryDebugView(Godot.Collections.Dictionary dictionary)
        {
            if (dictionary == null)
                throw new ArgumentNullException(nameof(dictionary));

            _dictionary = dictionary;
        }

        [DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
        public KeyValuePair<Variant, Variant>[] Items
        {
            get
            {
                KeyValuePair<Variant, Variant>[] items = new KeyValuePair<Variant, Variant>[_dictionary.Count];
                ((ICollection<KeyValuePair<Variant, Variant>>)_dictionary).CopyTo(items, 0);
                return items;
            }
        }
    }

    internal sealed class DictionaryDebugView<[MustBeVariant] TKey, [MustBeVariant] TValue>
    {
        private readonly Godot.Collections.Dictionary<TKey, TValue> _dictionary;

        public DictionaryDebugView(Godot.Collections.Dictionary<TKey, TValue> dictionary)
        {
            if (dictionary == null)
                throw new ArgumentNullException(nameof(dictionary));

            _dictionary = dictionary;
        }

        [DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
        public KeyValuePair<TKey, TValue>[] Items
        {
            get
            {
                KeyValuePair<TKey, TValue>[] items = new KeyValuePair<TKey, TValue>[_dictionary.Count];
                ((ICollection<KeyValuePair<TKey, TValue>>)_dictionary).CopyTo(items, 0);
                return items;
            }
        }
    }
}
