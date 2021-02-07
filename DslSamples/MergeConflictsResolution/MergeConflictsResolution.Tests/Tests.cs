using System.Collections.Generic;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.ProgramSynthesis.Wrangling.Tree;
using System;

namespace MergeConflictsResolution
{
    [TestClass]
    public class Tests
    {
        public IReadOnlyList<Node> PathToNode(List<string> path)
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
        public IReadOnlyList<Node>  resolution2string (string resolution)
        {
            List<Node> list = new List<Node>();
            string[] path = resolution.Split(
                                new[] { "\r\n", "\r", "\n" },
                                StringSplitOptions.None
                            );
            foreach (string pathValue in path)
            {
                Attributes.Attribute attr = new Attributes.Attribute("path", pathValue.Replace("\n", "").Replace("\\n", "").Replace("\r", "").Replace("#include", "").Replace(" ", "").Replace("\"", "").Replace("'", ""));
                Attributes.SetKnownSoftAttributes(new[] { "", "" });
                Node node = StructNode.Create("node1", new Attributes(attr));
                list.Add(node);
            }
            return list.AsReadOnly();
        }
        public MergeConflict convertConflict(string conflict, string fileContent, string filePath)
        {
            string Head_lookup = "<<<<<<< HEAD";
            string Middle_lookup = "=======";
            string End_lookup = ">>>>>>>";
            bool flag = false;
            string[] linesConflict = conflict.Split(
                                new[] { "\r\n", "\r", "\n" },
                                StringSplitOptions.None
                            );
            List<string> conflictListForked = new List<string>();
            List<string> conflictListMain = new List<string>();
            foreach (string line in linesConflict)
            {
                if (line.StartsWith("#include") && flag == false)
                {
                    string temp;
                    temp = line.Replace("\n", "").Replace("\\n", "").Replace("\r", "").Replace("#include", "").Replace(" ", "").Replace("\"", "").Replace("'", "");
                    conflictListMain.Add(temp);
                }
                if (line.StartsWith("#include") && flag == true)
                {
                    string temp;
                    temp = line.Replace("\n", "").Replace("\\n", "").Replace("\r", "").Replace("#include", "").Replace(" ", "").Replace("\"", "").Replace("'", "");
                    conflictListForked.Add(temp);
                }
                if (line.StartsWith(Head_lookup))
                    flag = true;
                if (line.StartsWith(Middle_lookup))
                    flag = false;
            }
            //File Content
            flag = false;
            string[] linesForkedFile = conflict.Split(
                                new[] { "\r\n", "\r", "\n" },
                                StringSplitOptions.None
                            );
            List<string> conflictForked = new List<string>();
            foreach (string line in linesConflict)
            {
                if (line.StartsWith("#include") && flag == false)
                {
                    string temp;
                    temp = line.Replace("\n", "").Replace("\\n", "").Replace("\r", "").Replace("#include", "").Replace(" ", "").Replace("\"", "").Replace("'", "");
                    conflictForked.Add(temp);
                }
                if (line.StartsWith(Head_lookup))
                    flag = true;
                if (line.StartsWith(End_lookup))
                    flag = false;
            }
            MergeConflict input = new MergeConflict(PathToNode(conflictListForked), PathToNode(conflictListMain), PathToNode(conflictForked), PathToNode(new List<string>()), filePath);

            return input;
        }
        public string conflict2Node(IReadOnlyList<Node> resolution)
        {
            string strResolution = "";
            foreach (Node n in resolution)
            {
                if (Semantics.Nodevalue(n, "path") != "")
                    strResolution= strResolution+ "# include \""+ Semantics.Nodevalue(n, "path")+"\n" +"\"";
            }
            return strResolution;
        }
        [TestMethod]
        public void LearnExample1c1d()
        {
            string conflict1 = "<<<<<<< HEAD\n" +
                             "#include \"base/command_line.h\"\n" +
                             "#include \"base/logging.h\"\n" +
                             "=======\n" +
                             "#include \"base/check_op.h\"\n"+
                             ">>>>>>>";
            string resolution1 = "#include \"base/command_line.h\"\n" +
                             "#include \"base/check_op.h\"";
            string conflict2 = "<<<<<<< HEAD\n" +
                             "#include \"base/logging.h\"\n" +
                             "#include \"base/scoped_native_library.h\"\n" +
                             "=======\n" +
                             "#include \"base/notreached.h\"\n" +
                             ">>>>>>>";
            string resolution2 = "#include \"base/scoped_native_library.h\"\n" +
                             "#include \"base/notreached.h\"";
            MergeConflict input1 = convertConflict(conflict1, "", "");
            IReadOnlyList<Node> output1 = resolution2string(resolution1);
            MergeConflict input2 = convertConflict(conflict2, "", "");
            IReadOnlyList<Node> output2 = resolution2string(resolution2);
            List<MergeConflictsResolutionExample> examples = new List<MergeConflictsResolutionExample>();
            examples.Add(new MergeConflictsResolutionExample(input1, output1));
            examples.Add(new MergeConflictsResolutionExample(input2, output2));
            Program program = Learner.Instance.Learn(examples);
            string s = program.Serialize();
            System.IO.File.WriteAllText("Program1c1d.txt", s);
            IReadOnlyList<Node> outputProg = program.Run(input1);
            Assert.AreEqual(true, Equal(output1,outputProg));
            IReadOnlyList<Node> test = program.Run(input1);
            IReadOnlyList<Node> outputProg2 = program.Run(input2);
            Assert.AreEqual(true, Equal(output2, outputProg2));
            ///program.Run(...);
        }
        [TestMethod]
        public void LearnExample1a1b()
        {
            string conflict1 = "<<<<<<< HEAD\n" +
                             "#include \"ui/base/anonymous_ui_base_features.h\"\n" +
                             "#include \"ui/base/mojom/cursor_type.mojom-shared.h\"\n" +
                             "=======\n" +
                             "#include \"ui/base/cursor/mojom/cursor_type.mojom-shared.h\"\n" +
                             ">>>>>>>";
            string resolution1 = "#include \"ui/base/anonymous_ui_base_features.h\"\n" +
                             "#include \"ui/base/mojom/cursor_type.mojom-shared.h\"";
            string conflict2 = "<<<<<<< HEAD\n" +
                             "#include \"ui/base/anonymous_ui_base_feature.h\"\n" +
                             "#include \"ui/base/mojom/cursor_type.mojom-blink.h\"\n" +
                             "=======\n" +
                             "#include \"ui/base/cursor/mojom/cursor_type.mojom-blink.h\"\n" +
                             ">>>>>>>";
            string resolution2 = "#include \"ui/base/anonymous_ui_base_feature.h\"\n" +
                             "#include \"ui/base/mojom/cursor_type.mojom-blink.h\"";
            MergeConflict input1 = convertConflict(conflict1, "", "");
            IReadOnlyList<Node> output1 = resolution2string(resolution1);
            MergeConflict input2 = convertConflict(conflict2, "", "");
            IReadOnlyList<Node> output2 = resolution2string(resolution2);
            List<MergeConflictsResolutionExample> examples = new List<MergeConflictsResolutionExample>();
            examples.Add(new MergeConflictsResolutionExample(input1, output1));
            examples.Add(new MergeConflictsResolutionExample(input2, output2));
            Program program = Learner.Instance.Learn(examples);
            string s = program.Serialize();
            System.IO.File.WriteAllText("Program1a1b.txt", s);
            IReadOnlyList<Node> outputProg = program.Run(input1);
            Assert.AreEqual(true, Equal(output1, outputProg));
            IReadOnlyList<Node> test = program.Run(input1);
            IReadOnlyList<Node> outputProg2 = program.Run(input2);
            Assert.AreEqual(true, Equal(output2, outputProg2));
            ///program.Run(...);
        }
        [TestMethod]
        public void TestExample1c1d()
        {
            List<string> forked = new List<string>();
            List<string> main = new List<string>();
            List<string> mainFilePath = new List<string>();
            List<string> forkedFilePath = new List<string>();
            List<string> resolved = new List<string>();
            forked.Add("base/logging.h");
            forked.Add("ui/base/anonymous_ui_base_features.h");
            main.Add("ui/base/cursor/mojom/cursor_type.mojom-shared.h");
            resolved.Add("ui/base/anonymous_ui_base_features.h");
            resolved.Add("ui/base/cursor/mojom/cursor_type.mojom-shared.h");
            string FindProgram = System.IO.File.ReadAllText(@"Program1c1d.txt");
            Program progFindProgram = Loader.Instance.Load(FindProgram);
            MergeConflict input = new MergeConflict(PathToNode(forked), PathToNode(main), PathToNode(mainFilePath), PathToNode(forkedFilePath), "");
            IReadOnlyList<Node> outputExpected = PathToNode(resolved);
            IReadOnlyList<Node> outputActual = progFindProgram.Run(input);
            Assert.AreEqual(true, Equal(outputExpected, outputActual));

        }
        public bool Equal(IReadOnlyList<Node> tree1, IReadOnlyList<Node> tree2)
        {
            bool ret = true;
            List<Node> retain = new List<Node>();
            foreach (Node n in tree1)
            {
                if (Semantics.Nodevalue(n, "path") != "")
                    retain.Add(n);
            }
            IReadOnlyList<Node> tree1Modified = retain.AsReadOnly();
            if (tree1Modified.Count != tree2.Count)
                return false;
            else
            {
                for (int i = 0; i < tree1Modified.Count; i++)
                {
                    if (Semantics.Nodevalue(tree1Modified[i], "path") != Semantics.Nodevalue(tree2[i], "path"))
                    {
                        ret = false;
                    }
                }
                return ret;
            }
        }
    }
}