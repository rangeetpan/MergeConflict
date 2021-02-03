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
            downpath.Add("B");
            resolved.Add("B");
            uppath2.Add("A");
            downpath2.Add("B");
            resolved2.Add("B");
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
            Assert.AreEqual(output, program.Run(input));
            IReadOnlyList<Node> test = program.Run(input);
            Assert.AreEqual(output2, program.Run(input2));
            ///program.Run(...);
        }
    }
}