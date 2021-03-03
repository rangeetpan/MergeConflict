using System;
using System.Collections.Generic;
using System.IO;
using MergeConflictsResolution;
using Microsoft.ProgramSynthesis.Wrangling.Tree;
using NUnit.Framework;
using static MergeConflictsResolution.Utils;
namespace MergeConflictsResolutionConsole
{
    class SampleDataset
    {
        public void EntireTest()
        {
            List<bool> checkOutput = new List<bool>();
            List<Program> programList = LoadProgram();
            string testcasePath = @"..\Program\TestCases\Test\";
            List<string> countTestCase = TestCaseLoad(testcasePath);
            foreach (string number in countTestCase)
            {
                MergeConflict input = LoadTestInput(testcasePath, number);
                checkOutput.Add(ValidOutput(input, programList, testcasePath, number));
            }
            List<bool> valid = checkOutput;
        }
    }
}
