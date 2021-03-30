using MergeConflictsResolution;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Microsoft.ProgramSynthesis.Wrangling.Tree;
using System.Linq;
using static MergeConflictsResolution.Utils;

namespace Tests
{
    class TestUtils
    {
        public static List<Program> LoadProgram()
        {
            List<Program> programs = new List<Program>();
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progMainSpecific.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progMainDuplicate.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progMainDuplicateNoMain.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progMainAndForkDuplicate.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progForkDuplicate.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progProjectSpecific.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progProjectSpecificNoMain.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progDependency.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progFork.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progMacroFork.txt")));
            programs.Add(Loader.Instance.Load(System.IO.File.ReadAllText(@"..\..\..\Programs\progMacroRename.txt")));
            return programs;
        }
        public static List<string> TestCaseLoad(string testcasePath, string particularTest = null)
        {
            List<string> countTestCase = new List<string>();
            if (particularTest != null)
            {
                countTestCase.Add(particularTest);
                return countTestCase;
            }
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
        public static MergeConflict LoadTestInput(string testcasePath, string number, string filePath, int type = 1)
        {
            string conflict = testcasePath + number + "_Conflict.txt";
            string content = testcasePath + number + "_FileName.txt";
            string fileContent = null;
            if (File.Exists(content))
            {
                content = System.IO.File.ReadAllText(@content);
                fileContent = filePath + content;
            }
            if (File.Exists(fileContent))
            {
                fileContent = System.IO.File.ReadAllText(@fileContent);
            }
            else
            {
                fileContent = null;
            }
            conflict = System.IO.File.ReadAllText(@conflict);
            MergeConflict input = new MergeConflict(conflict, fileContent, "", type);
            return input;
        }
        internal static List<IReadOnlyList<Node>> LoadAllSynthesizedCode(MergeConflict input, List<Program> programList, int type)
        {
            List<IReadOnlyList<Node>> outputQueue = new List<IReadOnlyList<Node>>();
            if (type == 1)
            {
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
            }
            else
            {
                outputQueue.Add(programList[9].Run(input));
                outputQueue.Add(programList[10].Run(input));
            }
            return outputQueue;
        }
        public static IReadOnlyList<Node> readResolvedConflicts(string excelPath, int type)
        {
            string resolvedConflict = System.IO.File.ReadAllText(@excelPath);
            if (type == 1)
            {
                string[] includePaths = resolvedConflict.Split(',');
                List<string> path = new List<string>();
                foreach (string includePath in includePaths)
                {
                    string temp;
                    temp = includePath.NormalizeInclude();
                    path.Add(temp);
                }
                return MergeConflict.PathToNode(path);
            }
            else
            {
                string temp;
                List<string> path = new List<string>();
                temp = resolvedConflict.NormalizeInclude();
                path.Add(temp);
                return MergeConflict.PathToNode(path);
            }
        }
        public static bool Equal(IReadOnlyList<Node> tree1, IReadOnlyList<Node> tree2)
        {
            List<string> retainTree1 = new List<string>();
            List<string> retainTree2 = new List<string>();
            foreach (Node n in tree1)
            {
                if (Semantics.NodeValue(n, "path") != "")
                    retainTree1.Add(Semantics.NodeValue(n, "path"));
            }
            foreach (Node n in tree2)
            {
                if (Semantics.NodeValue(n, "path") != "")
                    retainTree2.Add(Semantics.NodeValue(n, "path"));
            }
            return retainTree1.SequenceEqual(retainTree2);
        }
        public static bool ValidOutput(MergeConflict input, List<Program> programList, string testcasePath, string number)
        {
            string resolvedFilename = testcasePath + number + "_Resolved.txt";
            int typeOfConflict = conflictType(resolvedFilename);
            IReadOnlyList<Node> output = readResolvedConflicts(resolvedFilename, typeOfConflict);
            List<bool> checkOutput = new List<bool>();
            List<IReadOnlyList<Node>> outputQueue = LoadAllSynthesizedCode(input, programList, typeOfConflict);
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
