using System;
using System.Collections.Generic;
using System.IO;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace MergeConflictsResolution {
    public static class Utils {
        internal const string Include = "#include";

        internal const string Path = "path";

        internal static string NormalizeInclude(this string include) 
            => include.Replace("\n", "")
                .Replace("\\n", "")
                .Replace("\r", "")
                .Replace(Include, "")
                .Replace(" ", "")
                .Replace("\"", "")
                .Replace("'", "");

        internal static string[] SplitLines(this string s) 
            => s?.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.None) ?? new string[0];

        internal static string GetPath(this Node node) => node.Attributes.TryGetValue(Path, out string path) ? path : null;

        public static List<Program> LoadProgram()
        {
            List<Program> programs = new List<Program>();
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\Program\FindDownstreamSpecificTest.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\Program\FindDuplicateInDownstreamOutsideTest.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\Program\FindDuplicateInDownstreamOutsideTest2.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\Program\FindDuplicateInUpstreamAndDownstreamTest.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\Program\FindDuplicateInUpstreamOutsideTest.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\Program\FindFreqPatternTest.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\Program\FindFreqPatternTest2.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\Program\FindDendencyTest.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\Program\FindDownstream.txt")));
            return programs;
        }
        public static List<string> TestCaseLoad(string testcasePath)
        {
            List<string> countTestCase = new List<string>();
            foreach (string file in Directory.EnumerateFiles(testcasePath, "*.txt"))
            {
                int index = file.IndexOf(testcasePath);
                string cleanPath = (index < 0)
                    ? file
                    : file.Remove(index, testcasePath.Length);
                string number = cleanPath.Split('_')[0];
                if (countTestCase.Count == 0)
                    countTestCase.Add(number);
                else
                {
                    bool flag = false;
                    foreach (string testCaseNum in countTestCase)
                    {
                        if (testCaseNum == number)
                            flag = true;
                    }
                    if (flag == false)
                        countTestCase.Add(number);
                }
            }
            return countTestCase;
        }
        public static MergeConflict LoadTestInput(string testcasePath, string number)
        {
            string conflict = testcasePath + number + "_Conflict.txt";
            string content = testcasePath + number + "_FileName.txt";
            content = System.IO.File.ReadAllText(@content);
            conflict = System.IO.File.ReadAllText(@conflict);
            conflict = System.IO.File.ReadAllText(@conflict);
            MergeConflict input = new MergeConflict(conflict, content, "");
            return input;
        }
        public static List<IReadOnlyList<Node>> LoadOutput(MergeConflict input, List<Program> programList)
        {
            List<IReadOnlyList<Node>> outputQueue = new List<IReadOnlyList<Node>>();
            outputQueue.Add(programList[0].Run(input));
            outputQueue.Add(programList[1].Run(input));
            outputQueue.Add(programList[2].Run(input));
            outputQueue.Add(programList[3].Run(input));
            if (Semantics.NodeValue(input.Upstream[0], "path") != "")
                outputQueue.Add(programList[4].Run(input));
            outputQueue.Add(programList[5].Run(input));
            outputQueue.Add(programList[6].Run(input));
            outputQueue.Add(programList[7].Run(input));
            outputQueue.Add(programList[8].Run(input));
            return outputQueue;
        }
        public static IReadOnlyList<Node> Excel2String(string excelPath)
        {
            string excelCell = System.IO.File.ReadAllText(@excelPath);
            string[] includePaths = excelCell.Split(',');
            List<string> path = new List<string>();
            foreach (string includePath in includePaths)
            {
                string temp;
                temp = includePath.Replace("\\n", "").Replace("\r", "").Replace("#include", "").Replace(" ", "").Replace("\"", "").Replace("'", "");
                path.Add(temp);
            }
            return PathToNode(path);
        }
        public static IReadOnlyList<Node> PathToNode(List<string> path)
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
        public static bool Equal(IReadOnlyList<Node> tree1, IReadOnlyList<Node> tree2)
        {
            bool ret = true;
            List<Node> retain = new List<Node>();
            foreach (Node n in tree1)
            {
                if (Semantics.NodeValue(n, "path") != "")
                    retain.Add(n);
            }
            IReadOnlyList<Node> tree1Modified = retain.AsReadOnly();
            if (tree1Modified.Count != tree2.Count)
                return false;
            else
            {
                for (int i = 0; i < tree1Modified.Count; i++)
                {
                    if (Semantics.NodeValue(tree1Modified[i], "path") != Semantics.NodeValue(tree2[i], "path"))
                    {
                        ret = false;
                    }
                }
                return ret;
            }
        }
        public static bool ValidOutput(MergeConflict input, List<Program> programList, string testcasePath, string number)
        {
            string resolvedFilename = testcasePath + number + "_Resolved.txt";
            IReadOnlyList<Node> output = Excel2String(resolvedFilename);
            List<bool> checkOutput = new List<bool>();
            List<IReadOnlyList<Node>> outputQueue = LoadOutput(input, programList);
            bool flagStart = false;
            bool flagValid = false;
            foreach (IReadOnlyList<Node> n in outputQueue)
            {
                if (n != null)
                {
                    if (n.Count > 0)
                    {
                        flagStart = true;
                        if (Equal(n, output))
                            flagValid = true;
                    }
                }
            }
            if (flagValid)
                checkOutput.Add(true);
            else if (flagStart == false)
            {
                if (Equal(Semantics.Concat(input.Downstream, input.Upstream), output) == true || Equal(Semantics.Concat(input.Upstream, input.Downstream), output) == true)
                    checkOutput.Add(true);
                else if (Semantics.NodeValue(input.Upstream[0], "path") == "")
                {
                    if (Equal(input.Downstream, output) == true)
                    {
                        checkOutput.Add(true);
                    }
                    else
                        checkOutput.Add(false);
                }
                else
                    checkOutput.Add(false);
            }
            else
            {
                checkOutput.Add(false);
            }
            return checkOutput[0];

        }
    }
}
