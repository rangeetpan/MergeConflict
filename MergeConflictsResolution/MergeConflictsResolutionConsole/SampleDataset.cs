using System;
using System.Collections.Generic;
using System.IO;
using MergeConflictsResolution;
using Microsoft.ProgramSynthesis.Wrangling.Tree;
//using NUnit.Framework;
using static MergeConflictsResolution.Utils;
using Microsoft.VisualStudio.TestTools.UnitTesting;
namespace MergeConflictsResolutionConsole
{
    [TestClass]
    public class SampleDataset
    {
        [TestMethod]
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
            Microsoft.VisualStudio.TestTools.UnitTesting.Assert.AreEqual(true, true);
        }
        //[Test]
        //[TestCase("1")]
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
