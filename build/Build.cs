using System;
using System.IO;
using System.Linq;
using JsonConfig;
using Nuke.Common;
using Nuke.Common.BuildServers;
using Nuke.Common.Git;
using Nuke.Common.ProjectModel;
using Nuke.Common.Tools.GitVersion;
using Nuke.Common.Tools.MSBuild;
using Vestris.ResourceLib;
using static Nuke.Common.EnvironmentInfo;
using static Nuke.Common.IO.FileSystemTasks;
using static Nuke.Common.IO.PathConstruction;
using static Nuke.Common.Tools.MSBuild.MSBuildTasks;

class Build : NukeBuild
{
    public static int Main () => Execute<Build>(x => x.Compile);

    [Solution("VDX.sln")] readonly Solution Solution;
    [GitRepository] readonly GitRepository GitRepository;
    [GitVersion] private readonly GitVersion GitVersion;

    AbsolutePath SourceDirectory => RootDirectory / "src";
    AbsolutePath ArtifactsDirectory => RootDirectory / "artifacts";

    Target Clean => _ => _
        .Executes(() =>
        {
            DeleteDirectories(GlobDirectories(SourceDirectory, "**/bin", "**/obj"));
            EnsureCleanDirectory(ArtifactsDirectory);
        });

    Target Restore => _ => _
        .DependsOn(Clean)
        .Executes(() =>
        {
            MSBuild(s => s
                .SetTargetPath(Solution)
                .SetTargets("Restore"));
        });

    Target Compile => _ => _
        .DependsOn(Restore)
        .Executes(() =>
        {
            //
            // Build x64
            // 
            MSBuild(s => s
                .SetTargetPath(Solution)
                .SetTargets("Rebuild")
                .SetConfiguration(Configuration)
                .SetMaxCpuCount(Environment.ProcessorCount)
                .SetNodeReuse(IsLocalBuild)
                .SetTargetPlatform(MSBuildTargetPlatform.x64));

            //
            // Build x86
            // 
            MSBuild(s => s
                .SetTargetPath(Solution)
                .SetTargets("Rebuild")
                .SetConfiguration(Configuration)
                .SetMaxCpuCount(Environment.ProcessorCount)
                .SetNodeReuse(IsLocalBuild)
                .SetTargetPlatform(MSBuildTargetPlatform.x86));

            if (Configuration.Equals("release", StringComparison.InvariantCultureIgnoreCase))
            {
                var version =
                    new Version(IsLocalBuild ? GitVersion.GetNormalizedFileVersion() : AppVeyor.Instance.BuildVersion);

                StampVersion(
                    Path.Combine(RootDirectory, @"bin\x64\VDX.exe"),
                    version);

                StampVersion(
                    Path.Combine(RootDirectory, @"bin\x86\VDX.exe"),
                    version);
            }
        });

    private Target Pack => _ => _
        .DependsOn(Compile)
        .Executes(() =>
        {
            MSBuild(s => s
                .SetTargetPath(Solution)
                .SetTargets("Restore", "Pack")
                .SetPackageOutputPath(ArtifactsDirectory)
                .SetConfiguration(Configuration)
                .EnableIncludeSymbols());
        });

    private static void StampVersion(string path, Version version)
    {
        var versionResource = new VersionResource
        {
            FileVersion = version.ToString(),
            ProductVersion = version.ToString()
        };

        var stringFileInfo = new StringFileInfo();
        versionResource[stringFileInfo.Key] = stringFileInfo;
        var stringFileInfoStrings = new StringTable
        {
            LanguageID = 1033,
            CodePage = 1200
        };
        stringFileInfo.Strings.Add(stringFileInfoStrings.Key, stringFileInfoStrings);
        stringFileInfoStrings["CompanyName"] = Config.Global.Version.CompanyName;
        stringFileInfoStrings["FileDescription"] = Config.Global.Version.FileDescription;
        stringFileInfoStrings["FileVersion"] = version.ToString();
        stringFileInfoStrings["InternalName"] = Config.Global.Version.InternalName;
        stringFileInfoStrings["LegalCopyright"] = Config.Global.Version.LegalCopyright;
        stringFileInfoStrings["OriginalFilename"] = Config.Global.Version.OriginalFilename;
        stringFileInfoStrings["ProductName"] = Config.Global.Version.ProductName;
        stringFileInfoStrings["ProductVersion"] = version.ToString();

        var varFileInfo = new VarFileInfo();
        versionResource[varFileInfo.Key] = varFileInfo;
        var varFileInfoTranslation = new VarTable("Translation");
        varFileInfo.Vars.Add(varFileInfoTranslation.Key, varFileInfoTranslation);
        varFileInfoTranslation[ResourceUtil.USENGLISHLANGID] = 1300;

        versionResource.SaveTo(path);
    }
}
