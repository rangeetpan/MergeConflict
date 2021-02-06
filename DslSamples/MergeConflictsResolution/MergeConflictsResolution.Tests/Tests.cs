using System.Collections.Generic;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.ProgramSynthesis.Wrangling.Tree;
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
        [TestMethod]
        public void Example1c1d()
        {
            List<string> forked = new List<string>();
            List<string> main = new List<string>();
            List<string> upfilepath = new List<string>();
            List<string> downfilepath = new List<string>();
            List<string> resolved = new List<string>();
            List<string> forked2 = new List<string>();
            List<string> main2 = new List<string>();
            List<string> upfilepath2 = new List<string>();
            List<string> downfilepath2 = new List<string>();
            List<string> resolved2 = new List<string>();
            forked.Add("base/command_line.h");
            forked.Add("base/logging.h");
            main.Add("base/check_op.h");
            resolved.Add("base/command_line.h");
            resolved.Add("base/check_op.h");
            forked2.Add("base/logging.h");
            forked2.Add("base/scoped_native_library.h");
            main2.Add("base/notreached.h");
            resolved2.Add("base/scoped_native_library.h");
            resolved2.Add("base/notreached.h");
            MergeConflict input = new MergeConflict(PathToNode(forked), PathToNode(main), PathToNode(upfilepath), PathToNode(upfilepath), "");
            IReadOnlyList<Node> output = PathToNode(resolved);
            MergeConflict input2 = new MergeConflict(PathToNode(forked2), PathToNode(main2), PathToNode(upfilepath), PathToNode(upfilepath), "");
            IReadOnlyList<Node> output2 = PathToNode(resolved2);
            List<MergeConflictsResolutionExample> examples = new List<MergeConflictsResolutionExample>();
            examples.Add(new MergeConflictsResolutionExample(input, output));
            examples.Add(new MergeConflictsResolutionExample(input2, output2));
            Program program = Learner.Instance.Learn(examples);
            string s = program.Serialize();
            System.IO.File.WriteAllText("Program1c1d.txt", s);
            IReadOnlyList<Node> outputProg = program.Run(input);
            Assert.AreEqual(true, Equal(output,outputProg));
            IReadOnlyList<Node> test = program.Run(input);
            IReadOnlyList<Node> outputProg2 = program.Run(input2);
            Assert.AreEqual(true, Equal(output2, outputProg2));
            ///program.Run(...);
        }
        [TestMethod]
        public void Example1a1b()
        {
            List<string> forked = new List<string>();
            List<string> main = new List<string>();
            List<string> upfilepath = new List<string>();
            List<string> downfilepath = new List<string>();
            List<string> resolved = new List<string>();
            List<string> forked2 = new List<string>();
            List<string> main2 = new List<string>();
            List<string> upfilepath2 = new List<string>();
            List<string> downfilepath2 = new List<string>();
            List<string> resolved2 = new List<string>();
            forked.Add("ui/base/anonymous_ui_base_features.h");
            forked.Add("ui/base/mojom/cursor_type.mojom-shared.h");
            main.Add("ui/base/cursor/mojom/cursor_type.mojom-shared.h");
            resolved.Add("ui/base/anonymous_ui_base_features.h");
            resolved.Add("ui/base/mojom/cursor_type.mojom-shared.h");
            forked2.Add("ui/base/anonymous_ui_base_feature.h");
            forked2.Add("ui/base/mojom/cursor_type.mojom-blink.h");
            main2.Add("ui/base/cursor/mojom/cursor_type.mojom-blink.h");
            resolved2.Add("ui/base/anonymous_ui_base_feature.h");
            resolved2.Add("ui/base/mojom/cursor_type.mojom-blink.h");
            MergeConflict input = new MergeConflict(PathToNode(forked), PathToNode(main), PathToNode(upfilepath), PathToNode(upfilepath), "");
            IReadOnlyList<Node> output = PathToNode(resolved);
            MergeConflict input2 = new MergeConflict(PathToNode(forked2), PathToNode(main2), PathToNode(upfilepath), PathToNode(upfilepath), "");
            IReadOnlyList<Node> output2 = PathToNode(resolved2);
            List<MergeConflictsResolutionExample> examples = new List<MergeConflictsResolutionExample>();
            examples.Add(new MergeConflictsResolutionExample(input, output));
            examples.Add(new MergeConflictsResolutionExample(input2, output2));
            Program program = Learner.Instance.Learn(examples);
            string s = program.Serialize();
            System.IO.File.WriteAllText("Program1a1b.txt", s);
            IReadOnlyList<Node> outputProg = program.Run(input);
            Assert.AreEqual(true, Equal(output, outputProg));
            IReadOnlyList<Node> test = program.Run(input);
            IReadOnlyList<Node> outputProg2 = program.Run(input2);
            Assert.AreEqual(true, Equal(output2, outputProg2));
            ///program.Run(...);
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