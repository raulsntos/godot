using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

#nullable enable

namespace Godot
{
    public partial class Directory
    {
        // TODO: Need a replacement for DirectoryInfo, which is what this method would return.
        /// <summary>
        /// Retrieves the parent directory of the specified path,
        /// including both absolute and relative paths.
        /// </summary>
        /// <param name="path">The path for which to retrieve the parent directory.</param>
        /// <returns>
        /// The parent directory, or <see langword="null"/> if <paramref name="path"/>
        /// is the root directory.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        public static string? GetParent(string path)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            string parent = path.GetBaseDir();

            if (string.IsNullOrEmpty(parent))
            {
                return null;
            }

            return parent;
        }

        // TODO: Need a replacement for DirectoryInfo, which is what this method would return.
        /// <summary>
        /// Creates all directories and subdirectories in the specified path
        /// unless they already exist.
        /// </summary>
        /// <param name="path">The directory to create.</param>
        /// <returns>The path to the created directory.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// The directory specified by <paramref name="path"/> is a file.
        /// -or-
        /// An I/O error occurred while creating the directory.
        /// </exception>
        public static string CreateDirectory(string path)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            DirAccess.MakeDirRecursiveAbsolute(path).ThrowIfError();

            return path;
        }

        /// <summary>
        /// Determines whether the given path refers to an existing directory on disk.
        /// </summary>
        /// <param name="path">The path to test.</param>
        /// <returns>
        /// <see langword="true"/> if path refers to an existing directory;
        /// otherwise, <see langword="false"/>.
        /// </returns>
        public static bool Exists(string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                return false;
            }

            return DirAccess.DirExistsAbsolute(path);
        }

        private enum SearchKind
        {
            Files,
            Directories,
            All,
        }

        /// <summary>
        /// Returns the names of subdirectories (including their paths) in the
        /// specified directory.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <returns>
        /// An array of the full names (including paths) of subdirectories in the
        /// specified path, or an empty array if no directories are found.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static string[] GetDirectories(string path)
        {
            return GetDirectories(path, SearchOption.TopDirectoryOnly);
        }

        /// <summary>
        /// Returns the names of the subdirectories (including their paths) in
        /// the specified directory, and optionally searches subdirectories.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <param name="searchOption">
        /// One of the enumeration values that specifies whether the search
        /// operation should include all subdirectories or only the current
        /// directory.
        /// </param>
        /// <returns></returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static string[] GetDirectories(string path, SearchOption searchOption)
        {
            return EnumerateDirectories(path, searchOption).ToArray();
        }

        /// <summary>
        /// Returns the names of files (including their paths) in the specified
        /// directory.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <returns>
        /// An array of the full names (including paths) of subdirectories in the
        /// specified path, or an empty array if no directories are found.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static string[] GetFiles(string path)
        {
            return GetFiles(path, SearchOption.TopDirectoryOnly);
        }

        /// <summary>
        /// Returns the names of files (including their paths) in the specified
        /// directory, using a value to determine whether to search subdirectories.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <param name="searchOption">
        /// One of the enumeration values that specifies whether the search
        /// operation should include all subdirectories or only the current
        /// directory.
        /// </param>
        /// <returns>
        /// An array of the full names (including paths) of subdirectories in the
        /// specified path, or an empty array if no directories are found.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static string[] GetFiles(string path, SearchOption searchOption)
        {
            return EnumerateFiles(path, searchOption).ToArray();
        }

        /// <summary>
        /// Returns the names of all files and subdirectories in a specified path.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <returns>
        /// An array of file the file names and directory names, or an empty array
        /// if no files or directories are found.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static string[] GetFileSystemEntries(string path)
        {
            return GetFileSystemEntries(path, SearchOption.TopDirectoryOnly);
        }

        /// <summary>
        /// Returns an array of all the file names and directory names in a
        /// specified path, and optionally searches subdirectories.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <param name="searchOption">
        /// One of the enumeration values that specifies whether the search
        /// operation should include all subdirectories or only the current
        /// directory.
        /// </param>
        /// <returns>
        /// An array of file the file names and directory names, or an empty array
        /// if no files or directories are found.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static string[] GetFileSystemEntries(string path, SearchOption searchOption)
        {
            return EnumerateFileSystemEntries(path, searchOption).ToArray();
        }

        /// <summary>
        /// Returns an enumerable collection of directory full names in a specified path.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <returns>
        /// An enumerable collection of the full names (including paths) for the
        /// directories in the directory specified by <paramref name="path"/>.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static IEnumerable<string> EnumerateDirectories(string path)
        {
            return EnumerateDirectories(path, SearchOption.TopDirectoryOnly);
        }

        /// <summary>
        /// Returns an enumerable collection of directory full names in a
        /// specified path, and optionally searches subdirectories.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <param name="searchOption">
        /// One of the enumeration values that specifies whether the search
        /// operation should include all subdirectories or only the current
        /// directory.
        /// </param>
        /// <returns>
        /// An enumerable collection of the full names (including paths) for the
        /// directories in the directory specified by <paramref name="path"/>
        /// and that match the specified search option.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static IEnumerable<string> EnumerateDirectories(string path, SearchOption searchOption)
        {
            return EnumeratePathsCore(path, SearchKind.Directories, searchOption);
        }

        /// <summary>
        /// Returns an enumerable collection of full file names in a specified path.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <returns>
        /// An enumerable collection of the full names (including paths) for the
        /// files in the directory specified by <paramref name="path"/>.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static IEnumerable<string> EnumerateFiles(string path)
        {
            return EnumerateFiles(path, SearchOption.TopDirectoryOnly);
        }

        /// <summary>
        /// Returns an enumerable collection of full file names in a specified
        /// path, and optionally searches subdirectories.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <param name="searchOption">
        /// One of the enumeration values that specifies whether the search
        /// operation should include all subdirectories or only the current
        /// directory.
        /// </param>
        /// <returns>
        /// An enumerable collection of the full names (including paths) for the
        /// files in the directory specified by <paramref name="path"/>
        /// and that match the specified search option.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static IEnumerable<string> EnumerateFiles(string path, SearchOption searchOption)
        {
            return EnumeratePathsCore(path, SearchKind.Files, searchOption);
        }

        /// <summary>
        /// Returns an enumerable collection of file names and directory names
        /// in a specified path.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <returns>
        /// An enumerable collection of file-system entries in the directory
        /// specified by <paramref name="path"/>.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static IEnumerable<string> EnumerateFileSystemEntries(string path)
        {
            return EnumerateFileSystemEntries(path, SearchOption.TopDirectoryOnly);
        }

        /// <summary>
        /// Returns an enumerable collection of file names and directory names
        /// in a specified path, and optionally searches subdirectories.
        /// </summary>
        /// <param name="path">
        /// The relative or absolute path to the directory to search.
        /// </param>
        /// <param name="searchOption">
        /// One of the enumeration values that specifies whether the search
        /// operation should include all subdirectories or only the current
        /// directory.
        /// </param>
        /// <returns>
        /// An enumerable collection of file-system entries in the directory
        /// specified by path and that match the specified search option.
        /// </returns>
        /// <exception cref="IOException">
        /// An I/O error occurred while opening the file.
        /// </exception>
        public static IEnumerable<string> EnumerateFileSystemEntries(string path, SearchOption searchOption)
        {
            return EnumeratePathsCore(path, SearchKind.All, searchOption);
        }

        private static IEnumerable<string> EnumeratePathsCore(string path, SearchKind searchKind, SearchOption options)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            var dirQueue = new Queue<string>();
            dirQueue.Enqueue(path);

            while (dirQueue.TryDequeue(out string? currentDir))
            {
                using var dir = DirAccess.Open(currentDir);
                DirAccess.GetOpenError().ThrowIfError();

                dir.IncludeNavigational = false;
                dir.IncludeHidden = true;
                dir.ListDirBegin().ThrowIfError();

                string next;
                while (!string.IsNullOrEmpty(next = dir.GetNext()))
                {
                    next = currentDir.PathJoin(next);

                    bool isDir = dir.CurrentIsDir();

                    if (isDir)
                    {
                        if (options == SearchOption.AllDirectories)
                        {
                            dirQueue.Enqueue(next);
                        }

                        if (searchKind == SearchKind.Directories || searchKind == SearchKind.All)
                        {
                            yield return next;
                        }
                    }
                    else
                    {
                        if (searchKind == SearchKind.Files || searchKind == SearchKind.All)
                        {
                            yield return next;
                        }
                    }
                }

                dir.ListDirEnd();
            }
        }

        /// <summary>
        /// Returns the volume information, root information, or both for the
        /// specified path.
        /// </summary>
        /// <param name="path">The path of a file or directory.</param>
        /// <returns>
        /// A string that contains the volume information, root information,
        /// or both for the specified path.
        /// </returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        public static string GetDirectoryRoot(string path)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            string fullPath = Path.GetFullPath(path);
            string root = Path.GetPathRoot(fullPath)!;

            return root;
        }

        /// <summary>
        /// Gets the current working directory of the application.
        /// </summary>
        /// <returns>
        /// A string that contains the absolute path of the current working
        /// directory, and does not end with a backslash (<c>\</c>).
        /// </returns>
        public static string GetCurrentDirectory() => System.Environment.CurrentDirectory;

        /// <summary>
        /// Deletes an empty directory from a specified path.
        /// </summary>
        /// <param name="path">
        /// The name of the empty directory to remove. This directory must be
        /// writable and empty.
        /// </param>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// A file with the same name and location specified by
        /// <paramref name="path"/> already exists.
        /// -or-
        /// An I/O error occurred while deleting the directory.
        /// </exception>
        /// <exception cref="DirectoryNotFoundException">
        /// <paramref name="path"/> does not exist or could not be found.
        /// </exception>
        public static void Delete(string path)
        {
            ThrowIfParamPathIsNullOrEmpty(path, nameof(path));

            if (File.Exists(path))
            {
                throw new IOException($"A file with the same name and location already exists: '{path}'.");
            }

            if (!Exists(path))
            {
                throw new DirectoryNotFoundException($"The directory could not found: '{path}'.");
            }

            DirAccess.RemoveAbsolute(path).ThrowIfError();
        }

        /// <summary>
        /// Deletes the specified directory and, if indicated, any subdirectories
        /// and files in the directory.
        /// </summary>
        /// <param name="path">The name of the directory to remove.</param>
        /// <param name="recursive">
        /// <see langword="true"/> to remove directories, subdirectories, and
        /// files in path; otherwise, <see langword="false"/>.
        /// </param>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="path"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <paramref name="path"/> is a zero-length string or contains only
        /// white space.
        /// </exception>
        /// <exception cref="IOException">
        /// A file with the same name and location specified by
        /// <paramref name="path"/> already exists.
        /// -or-
        /// An I/O error occurred while deleting the directory.
        /// </exception>
        /// <exception cref="DirectoryNotFoundException">
        /// <paramref name="path"/> does not exist or could not be found.
        /// </exception>
        public static void Delete(string path, bool recursive)
        {
            if (!recursive)
            {
                Delete(path);
                return;
            }

            // TODO: Godot doesn't seem to have an API for deleting recursively.
            throw new NotImplementedException();
        }

        /// <summary>
        /// Retrieves the names of the logical drives on this computer in the
        /// form "&lt;drive letter&gt;:\".
        /// </summary>
        /// <returns>The logical drives on this computer.</returns>
        public static string[] GetLogicalDrives()
        {
            int driveCount = DirAccess.GetDriveCount();
            var logicalDrives = new string[driveCount];

            if (OperatingSystem.IsWindows())
            {
                for (int i = 0; i < driveCount; i++)
                {
                    logicalDrives[i] = $"{DirAccess.GetDriveName(i)}\\";
                }
            }
            else
            {
                for (int i = 0; i < driveCount; i++)
                {
                    logicalDrives[i] = DirAccess.GetDriveName(i);
                }
            }

            return logicalDrives;
        }

        private static void ThrowIfParamPathIsNullOrEmpty(string path, string paramName)
        {
            if (path == null)
            {
                throw new ArgumentNullException(paramName, "Directory name cannot be null.");
            }

            if (string.IsNullOrWhiteSpace(path))
            {
                throw new ArgumentException("Empty directory name is not legal.", paramName);
            }
        }
    }
}
