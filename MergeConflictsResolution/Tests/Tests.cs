using System.Collections.Generic;
using MergeConflictsResolution;
using NUnit.Framework;
using static MergeConflictsResolution.Utils;

namespace Tests
{
    [TestFixture]
    public class Tests
    {
        [Test]
        public void EntireTest()
        {
            List<bool> checkOutput = new List<bool>();
            List<Program> programList = LoadProgram();
            string testcasePath = @"..\Program\Dataset\IncludeSuite\";
            string filePath = @"..\Program\Dataset\Files\";
            List<string> countTestCase = TestCaseLoad(testcasePath);
            foreach (string number in countTestCase)
            {
                MergeConflict input = LoadTestInput(testcasePath, number, filePath);
                checkOutput.Add(ValidOutput(input, programList, testcasePath, number));
            }
            List<bool> valid = checkOutput;
        }

        [Test]
        public void IncludeTest(string fileName)
        {
            List<bool> checkOutput = new List<bool>();
            List<Program> programList = LoadProgram();
            string testcasePath = @"..\Program\Dataset\IncludeSuite\";
            string filePath = @"..\Program\Dataset\Files\";
            List<string> countTestCase = TestCaseLoad(testcasePath,fileName);
            foreach (string number in countTestCase)
            {
                MergeConflict input = LoadTestInput(testcasePath, number, filePath);
                checkOutput.Add(ValidOutput(input, programList, testcasePath, number));
            }
            List<bool> valid = checkOutput;
        }
    }

}
