﻿using System;
using System.Collections.Generic;
using Microsoft.ProgramSynthesis.Wrangling.Constraints;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace MergeConflictsResolution
{
    /// <summary>
    ///     Represents an example of a resolution, which includes 
    ///     (1) input: a merge conflict, and 
    ///     (2) output: a resolved text.
    /// </summary>
    public class ResolutionExample : Example<MergeConflict, IReadOnlyList<Node>>
    {
        private const string Include = "#include";
        public ResolutionExample(MergeConflict input, string resolved)
            : base(input, ResolutionToString(resolved)) { }

        private static IReadOnlyList<Node> ResolutionToString(string resolution)
        {
            List<Node> list = new List<Node>();
            string[] path = resolution.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.None);
            foreach (string pathValue in path)
            {
                Attributes.Attribute attr = new Attributes.Attribute("path", pathValue.Replace("\n", "").Replace("\\n", "").Replace("\r", "").Replace(Include, "").Replace(" ", "").Replace("\"", "").Replace("'", ""));
                Attributes.SetKnownSoftAttributes(new[] { "", "" });
                Node node = StructNode.Create("node1", new Attributes(attr));
                list.Add(node);
            }
            return list.AsReadOnly();
        }
    }
}
