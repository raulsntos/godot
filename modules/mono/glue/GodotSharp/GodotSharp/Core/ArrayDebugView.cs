using System;
using System.Diagnostics;

namespace Godot
{
    internal sealed class ArrayDebugView
    {
        private readonly Godot.Collections.Array _array;

        public ArrayDebugView(Godot.Collections.Array array)
        {
            if (array == null)
                throw new ArgumentNullException(nameof(array));

            _array = array;
        }

        [DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
        public Variant[] Items
        {
            get
            {
                Variant[] items = new Variant[_array.Count];
                _array.CopyTo(items, 0);
                return items;
            }
        }
    }

    internal sealed class ArrayDebugView<[MustBeVariant] T>
    {
        private readonly Godot.Collections.Array<T> _array;

        public ArrayDebugView(Godot.Collections.Array<T> array)
        {
            if (array == null)
                throw new ArgumentNullException(nameof(array));

            _array = array;
        }

        [DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
        public T[] Items
        {
            get
            {
                T[] items = new T[_array.Count];
                _array.CopyTo(items, 0);
                return items;
            }
        }
    }
}
