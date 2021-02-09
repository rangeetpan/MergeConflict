using System.Collections.Generic;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace MergeConflictsResolution
{
    [TestClass]
    public class Tests
    {
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
            MergeConflict input1 = new MergeConflict(conflict1);
            MergeConflict input2 = new MergeConflict(conflict2);
            List<ResolutionExample> examples = new List<ResolutionExample>
            {
                new ResolutionExample(input1, resolution1),
                new ResolutionExample(input2, resolution2)
            };

            Program program = Learner.Instance.Learn(examples);
            string s = program.Serialize();
            System.IO.File.WriteAllText("Program1c1d.txt", s);
            IReadOnlyList<Node> outputProg = program.Run(input1);
            Assert.AreEqual(true, Equal(output1,outputProg));
            IReadOnlyList<Node> test = program.Run(input1);
            IReadOnlyList<Node> outputProg2 = program.Run(input2);
            Assert.AreEqual(true, Equal(output2, outputProg2));
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
            MergeConflict input1 = new MergeConflict(conflict1);
            MergeConflict input2 = new MergeConflict(conflict2);
            List<ResolutionExample> examples = new List<ResolutionExample>();
            examples.Add(new ResolutionExample(input1, resolution1));
            examples.Add(new ResolutionExample(input2, resolution2));
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