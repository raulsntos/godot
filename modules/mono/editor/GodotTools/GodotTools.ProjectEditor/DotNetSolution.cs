using GodotTools.Core;
using Microsoft.VisualStudio.SolutionPersistence.Model;
using Microsoft.VisualStudio.SolutionPersistence.Serializer;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace GodotTools.ProjectEditor
{
    public class DotNetSolution
    {
        public static async Task GenAndSaveSolution(string dir, string name, string gameProjectPath, CancellationToken cancellationToken = default)
        {
            var solutionModel = new SolutionModel();
            solutionModel.AddPlatform("Any CPU");
            solutionModel.AddBuildType("Debug");
            solutionModel.AddBuildType("ExportDebug");
            solutionModel.AddBuildType("ExportRelease");
            solutionModel.AddProject(Path.GetRelativePath(dir, gameProjectPath));

            string solutionMoniker = Path.Combine(dir, $"{name}.sln");
            await SolutionSerializers.SlnFileV12.SaveAsync(solutionMoniker, solutionModel, cancellationToken);
        }

        public static async Task MigrateFromOldConfigNames(string slnPath, CancellationToken cancellationToken = default)
        {
            if (!File.Exists(slnPath))
                return;

            // We've only used the old config names in .sln files, so if the solution is .slnx or something else
            // we can assume we don't need to migrate.
            if (!slnPath.EndsWith(".sln"))
                return;

            var serializer = SolutionSerializers.GetSerializerByMoniker(slnPath);
            var solutionModel = await serializer.OpenAsync(slnPath, cancellationToken);

            if (!solutionModel.BuildTypes.Contains("Tools"))
                return;

            // This method renames old configurations in solutions to the new ones.
            //
            // These are the old configs and what we want to rename them to:
            //   Debug      ->    ExportDebug
            //   Release    ->    ExportRelease
            //   Tools      ->    Debug
            //
            // To rename them we remove the old config names and add the new ones,
            // but we want to add them in this order:
            //   Debug
            //   ExportDebug
            //   ExportRelease

            solutionModel.RemoveBuildType("Tools");
            solutionModel.RemoveBuildType("Release");

            solutionModel.AddBuildType("ExportRelease");
            solutionModel.AddBuildType("ExportDebug");

            // Save a copy of the solution before replacing it
            FileUtils.SaveBackupCopy(slnPath);

            await serializer.SaveAsync(slnPath, solutionModel, cancellationToken);
        }
    }
}
