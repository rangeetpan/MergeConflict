using System.Collections.Generic;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace MergeConflictsResolution
{
    [TestClass]
    public class Tests
    {
        [TestMethod]
        public void TestLearnSubstringPositiveAbsPos()
        {
            List<MergeConflictsResolutionExample> examples = new List<MergeConflictsResolutionExample>();
            Program program = Learner.Instance.Learn(examples);
            ///program.Run(...);
        }
    }
}