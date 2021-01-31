using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace ProseTutorial.synthesis
{
    public class MergeConflict
    {
#pragma warning disable IDE1006 // Naming Styles
        public string basePath { get; set; }
        public IReadOnlyList<Node> upstream { get; set; }
        public IReadOnlyList<Node> downstream { get; set; }
        public IReadOnlyList<Node> upstream_content { get; set; }
        public IReadOnlyList<Node> downstream_content { get; set; }

        public MergeConflict(IReadOnlyList<Node> upstream, IReadOnlyList<Node> downstream, IReadOnlyList<Node> upstream_content, IReadOnlyList<Node> downstream_content, global::System.String basePath)
        {
            this.upstream = upstream;
            this.downstream = downstream;
            this.upstream_content = upstream_content;
            this.downstream_content = downstream_content;
            this.basePath = basePath;
        }
    }
}
