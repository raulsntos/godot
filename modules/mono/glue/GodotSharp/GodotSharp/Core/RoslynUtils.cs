using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Godot
{
    internal static class RoslynUtils
    {
        /// <summary>
        /// Find a method with the given name in the given source code.
        /// </summary>
        /// <param name="method">Name of the method to look for.</param>
        /// <param name="sourceCode">Source code to look into.</param>
        /// <returns>The line position for the found method, or <c>-1</c> if not found.</returns>
        private static int FindMethod(string method, string sourceCode)
        {
            var tree = CSharpSyntaxTree.ParseText(sourceCode);

            var methodDeclarations = tree.GetRoot().DescendantNodes().OfType<MethodDeclarationSyntax>();
            foreach (var mds in methodDeclarations)
            {
                if (mds.Identifier.ValueText == method)
                {
                    var location = mds.GetLocation();
                    return location.GetLineSpan().StartLinePosition.Line;
                }
            }

            return -1;
        }

        /// <summary>
        /// Find a field or property with the given name in the given source code.
        /// </summary>
        /// <param name="member">Name of the field or property to look for.</param>
        /// <param name="sourceCode">Source code to look into.</param>
        /// <returns>The line position for the found member, or <c>-1</c> if not found.</returns>
        private static int FindMember(string member, string sourceCode)
        {
            var tree = CSharpSyntaxTree.ParseText(sourceCode);

            Location location = null;
            var memberDeclarations = tree.GetRoot().DescendantNodes().OfType<MemberDeclarationSyntax>();
            foreach (var mds in memberDeclarations)
            {
                if (mds is PropertyDeclarationSyntax pds)
                {
                    if (pds.Identifier.ValueText == member)
                    {
                        location = pds.GetLocation();
                        break;
                    }
                }
                else if (mds is FieldDeclarationSyntax fds)
                {
                    foreach (var variable in fds.Declaration.Variables)
                    {
                        if (variable.Identifier.ValueText == member)
                        {
                            location = variable.GetLocation();
                            break;
                        }
                    }
                }
            }

            if (location != null)
            {
                return location.GetLineSpan().StartLinePosition.Line;
            }

            return -1;
        }

        /// <summary>
        /// Add the given method to the given source code.
        /// </summary>
        /// <param name="className">Name of the class to add the method to.</param>
        /// <param name="methodCode">The method code that will be added to the class.</param>
        /// <param name="sourceCode">The source code that contains the class that will be modified.</param>
        /// <returns>
        /// The modified source code with the method added to the class,
        /// or the unmodified source code if the class was not found.
        /// </returns>
        private static string AddMethod(string className, string methodCode, string sourceCode)
        {
            var tree = CSharpSyntaxTree.ParseText(sourceCode);

            var cds = tree.GetRoot().DescendantNodes()
                .OfType<ClassDeclarationSyntax>()
                .Where(cds => cds.Identifier.ValueText == className)
                .FirstOrDefault();

            if (cds != null)
            {
                var mds = SyntaxFactory.ParseMemberDeclaration(methodCode);
                if (mds != null)
                {
                    // TODO: It would be nice to write the member with the proper indentation
                    // mds = mds.NormalizeWhitespace().WithTriviaFrom(cds);
                    var newCds = cds.AddMembers(mds);
                    var newTree = tree.GetRoot().ReplaceNode(cds, newCds);
                    return newTree.ToString();
                }
            }

            return sourceCode;
        }
    }
}
