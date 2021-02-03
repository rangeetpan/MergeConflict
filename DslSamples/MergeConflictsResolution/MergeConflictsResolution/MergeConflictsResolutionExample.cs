using System;
using System.Collections.Generic;
using Microsoft.ProgramSynthesis.Wrangling.Constraints;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace MergeConflictsResolution
{
    public class MergeConflictsResolutionExample : Example<MergeConflict, IReadOnlyList<Node>>
    {
        public MergeConflictsResolutionExample(MergeConflict input, IReadOnlyList<Node> resolved) 
            : base(input, resolved) { }
    }
}
