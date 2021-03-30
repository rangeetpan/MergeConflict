using System.Collections.Generic;
using Microsoft.ProgramSynthesis.Wrangling.Tree;
using static MergeConflictsResolution.Utils;

namespace MergeConflictsResolution
{
    /// <summary>
    ///     Class to represent a merge conflict.
    /// </summary>
    public class MergeConflict
    {
        /// <summary>
        ///     Constructs a merge conflict object.
        /// </summary>
        /// <param name="conflict">The merge conflict text.</param>
        /// <param name="fileContent">The optional file content.</param>
        /// <param name="filePath">The optional file path.</param>
        public MergeConflict(string conflict, string fileContent = null, string filePath = null, int type=1)
        {
            string headLookup = "<<<<<<< HEAD";
            string middleLookup = "=======";
            string endLookup = ">>>>>>>";

            bool inHeadSection = false;
            string[] linesConflict = conflict.SplitLines();
            List<string> conflictsInForked = new List<string>();
            List<string> conflictsInMain = new List<string>();
            foreach (string line in linesConflict)
            {
                if (type == 1)
                {
                    if (line.StartsWith(Include) && inHeadSection == false)
                    {
                        conflictsInMain.Add(line.NormalizeInclude());
                    }

                    if (line.StartsWith(Include) && inHeadSection == true)
                    {
                        conflictsInForked.Add(line.NormalizeInclude());
                    }
                }
                else
                {
                    if (!(line.Contains(headLookup) || line.Contains(middleLookup) || line.Contains(endLookup)))
                    {
                        if (inHeadSection == false)
                        {
                            conflictsInMain.Add(line.NormalizeInclude());
                        }

                        if (inHeadSection == true)
                        {
                            conflictsInForked.Add(line.NormalizeInclude());
                        }
                    }
                }
                if (line.StartsWith(headLookup))
                {
                    inHeadSection = true;
                }

                if (line.StartsWith(middleLookup))
                {
                    inHeadSection = false;
                }
            }

            string[] linesContent = fileContent.SplitLines();
            bool isOutsideConflicts = true;
            List<string> outsideConflictContent = new List<string>();
            foreach (string line in linesContent)
            {
                if (line.StartsWith(Include) && isOutsideConflicts)
                {
                    outsideConflictContent.Add(line.NormalizeInclude());
                }

                if (line.StartsWith(headLookup))
                {
                    isOutsideConflicts = false;
                }

                if (line.StartsWith(endLookup))
                {
                    isOutsideConflicts = true;
                }
            }
            if(conflictsInForked.Count==0)
            {
                conflictsInForked.Add("");
            }
            if (conflictsInMain.Count == 0)
            {
                conflictsInMain.Add("");
            }
            this.Upstream = PathToNode(conflictsInForked);
            this.Downstream = PathToNode(conflictsInMain);
            this.UpstreamContent = PathToNode(outsideConflictContent);
            this.BasePath = filePath;
        }

        internal string BasePath { get; }

        public IReadOnlyList<Node> Upstream { get; }

        public IReadOnlyList<Node> Downstream { get; }

        public IReadOnlyList<Node> UpstreamContent { get; }

        public static IReadOnlyList<Node> PathToNode(List<string> path)
        {
            List<Node> list = new List<Node>();
            foreach (string pathValue in path)
            {
                Attributes.Attribute attr = new Attributes.Attribute(Path, pathValue);
                Attributes.SetKnownSoftAttributes(new[] { "", "" });
                Node node = StructNode.Create("node1", new Attributes(attr));
                list.Add(node);
            }
            return list.AsReadOnly();
        }
    }
}
