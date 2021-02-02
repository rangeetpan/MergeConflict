using System.Collections.Generic;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace MergeConflictsResolution
{
    public class MergeConflict
    {
        public string BasePath { get; set; }

        public IReadOnlyList<Node> Upstream { get; set; }

        public IReadOnlyList<Node> Downstream { get; set; }

        public IReadOnlyList<Node> Upstream_content { get; set; }

        public IReadOnlyList<Node> Downstream_content { get; set; }

        public MergeConflict(IReadOnlyList<Node> upstream, 
                             IReadOnlyList<Node> downstream, 
                             IReadOnlyList<Node> upstream_content, 
                             IReadOnlyList<Node> downstream_content, 
                             string basePath)
        {
            this.Upstream = upstream;
            this.Downstream = downstream;
            this.Upstream_content = upstream_content;
            this.Downstream_content = downstream_content;
            this.BasePath = basePath;
        }
    }
}
