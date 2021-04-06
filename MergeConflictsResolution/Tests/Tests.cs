using System.Collections.Generic;
using MergeConflictsResolution;
using NUnit.Framework;
using static MergeConflictsResolution.Utils;
using Tests;

namespace Tests
{
    [TestFixture]
    public class Tests
    {
        [Test]
        public void IncludeTest()
        {
            List<bool> checkOutput = new List<bool>();
            List<Program> programList = TestUtils.LoadProgram();
            string testcasePath = @"..\..\..\..\..\Dataset\IncludeSuite";
            string filePath = @"..\..\..\..\..\Dataset\Files\";
            List<string> countTestCase= TestUtils.TestCaseLoad(testcasePath);
            foreach (string number in countTestCase)
            {
                MergeConflict input = TestUtils.LoadTestInput(testcasePath, number, filePath);
                Assert.IsTrue(TestUtils.ValidOutput(input, programList, testcasePath, number));
            }
        }
        [Test]
        public void MacroTest()
        {
            List<bool> checkOutput = new List<bool>();
            List<Program> programList = TestUtils.LoadProgram();
            string testcasePath = @"..\..\..\..\..\Dataset\MacroSuite";
            string filePath = @"..\..\..\..\..\Dataset\Files\";
            List<string> countTestCase = TestUtils.TestCaseLoad(testcasePath);
            foreach (string number in countTestCase)
            {
                MergeConflict input = TestUtils.LoadTestInput(testcasePath, number, filePath);
                Assert.IsTrue(TestUtils.ValidOutput(input, programList, testcasePath, number));
            }
        }
    }

}
