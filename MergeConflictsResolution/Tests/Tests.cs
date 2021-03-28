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
        public void IncludeTest()
        {
            List<bool> checkOutput = new List<bool>();
            List<Program> programList = LoadProgramInclude();
            string testcasePath = @"..\..\..\..\..\Dataset\IncludeSuite\";
            string filePath = @"..\..\..\..\..\Dataset\Files\";
            List<string> countTestCase = TestCaseLoad(testcasePath);
            List<string> abc = new List<string>();
            foreach (string number in countTestCase)
            {
                abc.Add(number);
                MergeConflict input = LoadTestInput(testcasePath, number, filePath);
                checkOutput.Add(ValidOutput(input, programList, testcasePath, number, 1));
            }
            List<bool> valid = checkOutput;
        }
        [Test]
        public void MacroTest()
        {
            List<bool> checkOutput = new List<bool>();
            List<Program> programList = LoadProgramMacro();
            string testcasePath = @"..\..\..\..\..\Dataset\MacroSuite\";
            string filePath = @"..\..\..\..\..\Dataset\Files\";
            List<string> countTestCase = TestCaseLoad(testcasePath);
            foreach (string number in countTestCase)
            {
                MergeConflict input = LoadTestInput(testcasePath, number, filePath, 2);
                checkOutput.Add(ValidOutput(input, programList, testcasePath, number,2));
            }
            List<bool> valid = checkOutput;
        }

        
        public void IncludeTest(string fileName)
        {
            List<bool> checkOutput = new List<bool>();
            List<Program> programList = LoadProgramInclude();
            string testcasePath = @"..\Program\Dataset\IncludeSuite\";
            string filePath = @"..\Program\Dataset\Files\";
            List<string> countTestCase = TestCaseLoad(testcasePath,fileName);
            foreach (string number in countTestCase)
            {
                MergeConflict input = LoadTestInput(testcasePath, number, filePath);
                checkOutput.Add(ValidOutput(input, programList, testcasePath, number,1));
            }
            List<bool> valid = checkOutput;
        }
    }

}
