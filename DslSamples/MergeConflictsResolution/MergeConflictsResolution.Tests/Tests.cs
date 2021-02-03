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
        public void TestLearnSubstringPositiveAbsPos()
        {
            List<string> uppath = new List<string>();
            List<string> downpath = new List<string>();
            List<string> upfilepath = new List<string>();
            List<string> downfilepath = new List<string>();
            List<string> resolved = new List<string>();
            List<string> uppath2 = new List<string>();
            List<string> downpath2 = new List<string>();
            List<string> upfilepath2 = new List<string>();
            List<string> downfilepath2 = new List<string>();
            List<string> resolved2 = new List<string>();
            uppath.Add("A");
            uppath.Add("C");
            //downpath.Add("B");
            resolved.Add("C");
            //resolved.Add("B");
            uppath2.Add("D");
            uppath2.Add("E");
            //downpath2.Add("B");
            resolved2.Add("E");
            MergeConflict input = new MergeConflict(PathToNode(uppath), PathToNode(downpath), PathToNode(upfilepath), PathToNode(upfilepath), "");
            IReadOnlyList<Node> output = PathToNode(resolved);
            MergeConflict input2 = new MergeConflict(PathToNode(uppath2), PathToNode(downpath2), PathToNode(upfilepath), PathToNode(upfilepath), "");
            IReadOnlyList<Node> output2 = PathToNode(resolved2);
            List<MergeConflictsResolutionExample> examples = new List<MergeConflictsResolutionExample>();
            examples.Add(new MergeConflictsResolutionExample(input, output));
            examples.Add(new MergeConflictsResolutionExample(input2, output2));
            Program program = Learner.Instance.Learn(examples);
            string s = program.Serialize();
            System.IO.File.WriteAllText("1.txt", s);
            IReadOnlyList<Node> outputProg = program.Run(input);
            Assert.AreEqual(true, Equal(output,outputProg));
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