using System;
using System.Collections.Generic;
using Microsoft.ProgramSynthesis.Wrangling.Constraints;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace MergeConflictsResolution
{
    public class MergeConflictsResolutionExample : Example<MergeConflict, IReadOnlyList<Node>>
    {
        public MergeConflictsResolutionExample(MergeConflict input, string resolved) 
            : base(input, ParseOutput(resolved)) { }

        private static IReadOnlyList<Node> ParseOutput(string output)
        {
            throw new NotImplementedException();
        }
    }
}
