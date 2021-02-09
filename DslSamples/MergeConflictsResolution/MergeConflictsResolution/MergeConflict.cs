using System;
using System.Collections.Generic;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace MergeConflictsResolution
{
    public class MergeConflict
    {
        public string BasePath { get; set; }

        public IReadOnlyList<Node> Upstream { get; set; }

        public IReadOnlyList<Node> Downstream { get; set; }

        public IReadOnlyList<Node> UpstreamContent { get; set; }

        public IReadOnlyList<Node> DownstreamContent { get; set; }

        public MergeConflict(string conflict, string fileContent = null, string filePath = null)
        {
            string Normalize(string line)
            {
                return line.Replace("\n", "").Replace("\\n", "").Replace("\r", "").Replace("#include", "").Replace(" ", "").Replace("\"", "").Replace("'", "");
            }

            string Head_lookup = "<<<<<<< HEAD";
            string Middle_lookup = "=======";
            string End_lookup = ">>>>>>>";
            bool flag = false;
            string[] linesConflict = conflict.Split(new[] { "\r\n", "\r", "\n" },StringSplitOptions.None);
            List<string> conflictListForked = new List<string>();
            List<string> conflictListMain = new List<string>();
            foreach (string line in linesConflict)
            {
                if (line.StartsWith("#include") && flag == false)
                {
                    conflictListMain.Add(Normalize(line));
                }

                if (line.StartsWith("#include") && flag == true)
                {
                    conflictListForked.Add(Normalize(line));
                }

                if (line.StartsWith(Head_lookup))
                {
                    flag = true;
                }

                if (line.StartsWith(Middle_lookup))
                {
                    flag = false;
                }
            }

            //File Content
            flag = false;
            List<string> conflictForked = new List<string>();
            foreach (string line in linesConflict)
            {
                if (line.StartsWith("#include") && flag == false)
                {
                    conflictForked.Add(Normalize(line));
                }

                if (line.StartsWith(Head_lookup))
                {
                    flag = true;
                }

                if (line.StartsWith(End_lookup))
                {
                    flag = false;
                }
            }

            this.Upstream = PathToNode(conflictListForked);
            this.Downstream = PathToNode(conflictListMain);
            this.UpstreamContent = PathToNode(conflictForked);
            this.DownstreamContent = PathToNode(new List<string>());
            this.BasePath = filePath;
        }

        private static IReadOnlyList<Node> PathToNode(List<string> path)
        {
            List<Node> list = new List<Node>();
            foreach (string pathValue in path)
            {
                Attributes.Attribute attr = new Attributes.Attribute("path", pathValue);
                Attributes.SetKnownSoftAttributes(new[] { "", "" });
                Node node = StructNode.Create("node1", new Attributes(attr));
                list.Add(node);
            }
            return list.AsReadOnly();
        }
    }
}
