using System.Collections.Generic;
using System.Linq;
using Microsoft.ProgramSynthesis.Utils;
using Microsoft.ProgramSynthesis.Utils.Interactive;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace MergeConflictsResolution
{
    /// <summary>
    ///     The implementations of the operators in the language.
    /// </summary>
    public static class Semantics
    {
        private static readonly string[] projectSpecificKeywords = { "edge", "microsoft", "EDGE" };
        private const string Path = "path";

        public static IReadOnlyList<Node> Apply(bool pattern, IReadOnlyList<Node> action)
        {
            return pattern == true ? action : null;
        }

        /// <summary>
        /// Removes list of selected Nodes from the input list.
        /// </summary>
        /// <param name="input"></param>
        /// <param name="selected"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> Remove(IReadOnlyList<Node> input, IReadOnlyList<Node> selected)
        {
            List<Node> result = input.ToList();
            List<int> removedIndices = new List<int>();
            int returnIndex;
            foreach (Node n in selected)
            {
                returnIndex = IndexNode(input, n);
                if (returnIndex != -1)
                {
                    removedIndices.Add(returnIndex);
                }
            }

            foreach (int index in removedIndices.OrderByDescending(v => v))
            {
                result.RemoveAt(index);
            }

            return result;
        }

        /// <summary>
        /// Returns index of the selected Node in the list.
        /// </summary>
        /// <param name="input"></param>
        /// <param name="selected"></param>
        /// <returns></returns>
        public static int IndexNode(IReadOnlyList<Node> input, Node selected)
        {
            string attrValue = NodeValue(selected, Path);
            int k = 0;

            foreach (Node n in input)
            {
                if (NodeValue(n, Path) == attrValue)
                {
                    return k;
                }

                k++;
            }

            return -1;
        }

        /// <summary>
        /// Joins two set of lines in the conflicts (List of nodes).
        /// </summary>
        /// <param name="input1"></param>
        /// <param name="input2"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> Concat(IReadOnlyList<Node> input1, IReadOnlyList<Node> input2)
        {
            return input1.Concat(input2).ToList();
        }

        /// <summary>
        /// Selects the upstream line by either index.
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <param name="k"></param>
        /// <returns></returns>
        /// Write own class 
        public static IReadOnlyList<Node> SelectUpstreamIdx(MergeConflict x, int k)
        {
            List<Node> result = new List<Node>();
            if (x.Upstream.Count > k)
            {
                result.Add(x.Upstream[k]);
            }

            return result;
        }

        /// <summary>
        /// visits all the nodes
        /// </summary>
        /// <param name="node"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> AllNodes(Node node)
        {
            var visitor = new GetAllNodesPostOrderVisitor();
            node.AcceptVisitor(visitor);
            return visitor.Nodes.ToArray();
        }

        /// <summary>
        /// select the downstream line by either index 
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <param name="k"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectDownstreamIdx(MergeConflict x, int k)
        {
            List<Node> result = new List<Node>();
            if (x.Downstream.Count > k)
            {
                result.Add(x.Downstream[k]);
            }

            return result;
        }

        /// <summary>
        /// Select downstream.
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectDownstream(MergeConflict x)
        {
            return x.Downstream;
        }

        /// <summary>
        /// Select upstream
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectUpstream(MergeConflict x)
        {
            return x.Upstream;
        }

        /// <summary>
        /// Select the node with the specified path (upstream)
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <param name="k"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectDownstreamPath(MergeConflict x, string k)
        {
            return SelectPath(x.Downstream, k);
        }

        /// <summary>
        /// Select the node with the specified path (downstream)
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <param name="k"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectUpstreamPath(MergeConflict x, string k)
        {
            return SelectPath(x.Upstream, k);
        }

        private static IReadOnlyList<Node> SelectPath(IReadOnlyList<Node> list, string k)
        {
            List<Node> result = new List<Node>();
            foreach (Node node in list)
            {
                if (node.Attributes.TryGetValue(Path, out string ret) && ret == k)
                {
                    result.Add(node);
                }
            }

            return result;
        }

        /// <summary>
        /// returns list of matched nodes.
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <param name="paths"></param>
        /// <returns></returns>
        public static List<IReadOnlyList<Node>> FindMatch(MergeConflict x, string[] paths)
        {
            List<IReadOnlyList<Node>> result = new List<IReadOnlyList<Node>>
            {
                FindDuplicateInUpstreamAndDownstream(x),
                FindFreqPattern(x, paths),
                FindDuplicateInDownstreamOutside(x),
                FindDuplicateInUpstreamOutside(x),
                FindUpstreamSpecific(x),
                FindDownstreamSpecific(x)
            };

            return result;
        }

        /// <summary>
        /// Validates if enabled predicate is present or not.
        /// </summary>
        /// <param name="dub"></param>
        /// <param name="enabledPredicate"></param>
        /// <returns></returns>
        public static bool Check(List<IReadOnlyList<Node>> dub, int[] enabledPredicate)
        {
            return enabledPredicate.Any(predicateCheck => dub[predicateCheck].Count <= 0);
        }

        /// <summary>
        /// selects the k-th element from the "dup".
        /// </summary>
        /// <param name="dup"></param>
        /// <param name="k"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectDup(List<IReadOnlyList<Node>> dup, int k)
        {
            return dup[k];
        }

        /// <summary>
        /// Identify duplicate headers inside the conflicting region
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <returns></returns>
        public static List<Node> FindDuplicateInUpstreamAndDownstream(MergeConflict x)
        {
            List<Node> nodes = new List<Node>();
            foreach (Node upstream in x.Upstream)
            {
                string upstreamValue = NodeValue(upstream, Path);
                foreach (Node downstream in x.Downstream)
                {
                    string downstreamValue = NodeValue(downstream, Path);
                    if (IsMatchPath(upstreamValue, downstreamValue) || IsMatchContent(upstreamValue, downstreamValue))
                    {
                        nodes.Add(upstream);
                    }
                }
            }

            return nodes;
        }

        /// <summary>
        /// identifies the project specific pattern
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <param name="paths"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> FindFreqPattern(MergeConflict x, string[] paths)
        {
            IEnumerable<Node> nodes = from stream in Concat(x.Downstream, x.Upstream)
                                      from path in paths
                                      where NodeValue(stream, Path) == path
                                      select stream;
            return nodes.ToList();
        }

        /// <summary>
        /// Identify duplicate headers outside the conflicting region (downstream specific)
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <returns></returns>
        public static List<Node> FindDuplicateInDownstreamOutside(MergeConflict x)
        {
            List<Node> nodes = new List<Node>();
            IReadOnlyList<Node> downstreamFileIncludeAst = x.UpstreamContent;
            foreach (Node downstream in x.Downstream)
            {
                string downstreamValue = NodeValue(downstream, Path);
                foreach (Node outsideInclude in downstreamFileIncludeAst)
                {
                    string outsideIncludeValue = NodeValue(outsideInclude, Path);
                    if (IsMatchPath(outsideIncludeValue, downstreamValue) || IsMatchContent(outsideIncludeValue, downstreamValue))
                    {
                        nodes.Add(downstream);
                    }
                }
            }

            return nodes;
        }

        /// <summary>
        /// Identify duplicate headers outside the conflicting region (upstream specific)
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <returns></returns>
        public static List<Node> FindDuplicateInUpstreamOutside(MergeConflict x)
        {
            List<Node> nodes = new List<Node>();
            IReadOnlyList<Node> upstreamFileIncludeAst = x.UpstreamContent;
            foreach (Node upstream in x.Upstream)
            {
                string upstreamValue = NodeValue(upstream, Path);
                foreach (Node outsideInclude in upstreamFileIncludeAst)
                {
                    string outsideIncludeValue = NodeValue(outsideInclude, Path);
                    if (IsMatchPath(outsideIncludeValue, upstreamValue) || IsMatchContent(outsideIncludeValue, upstreamValue))
                    {
                        nodes.Add(upstream);
                    }
                }
            }

            return nodes;
        }

        /// <summary>
        /// identify the upstream specific header path
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <returns></returns>
        public static List<Node> FindUpstreamSpecific(MergeConflict x)
        {
            List<Node> nodes = new List<Node>();
            foreach (Node upstream in x.Upstream)
            {
                string upstreamValue = NodeValue(upstream, Path);
                if (!upstreamValue.Contains(".h"))
                {
                    bool flag = false;
                    string upstreamSplit = upstreamValue.Split('(')[0];
                    foreach (Node downstream in x.Downstream)
                    {
                        string downstreamValue = NodeValue(downstream, Path);
                        if (upstreamSplit != downstreamValue.Split('(')[0])
                        {
                            flag = true;
                            break;
                        }
                    }

                    if (flag == true)
                    {
                        if (projectSpecificKeywords.Any(s => upstreamValue.Contains(s)))
                        {
                            nodes.Add(upstream);
                        }
                    }
                }
                else if (projectSpecificKeywords.Any(s => upstreamValue.Contains(s)))
                {
                    nodes.Add(upstream);
                }
            }

            return nodes;
        }

        /// <summary>
        /// identify the downstream specific header path 
        /// </summary>
        /// <param name="x">The input merge conflict.</param>
        /// <returns></returns>
        public static List<Node> FindDownstreamSpecific(MergeConflict x)
        {
            List<Node> nodes = new List<Node>();
            foreach (Node downstream in x.Downstream)
            {
                if (!NodeValue(downstream, Path).Contains(".h"))
                {
                    bool flag = false;
                    foreach (Node upstream in x.Upstream)
                    {
                        if (NodeValue(downstream, Path).Split('(')[0] == NodeValue(upstream, Path).Split('(')[0])
                        {
                            flag = true;
                        }
                    }
                    if (flag == true)
                    {
                        if (projectSpecificKeywords.Any(s => NodeValue(downstream, Path).Contains(s)))
                            nodes.Add(downstream);
                    }
                }
                else if (projectSpecificKeywords.Any(s => NodeValue(downstream, Path).Contains(s)))
                    nodes.Add(downstream);
            }
            return nodes;
        }



        /// <summary>
        /// validates if the list of node empty or not.
        /// </summary>
        /// <param name="l"></param>
        /// <returns></returns>
        public static bool Match(IReadOnlyList<Node> l)
        {
            return l.Count != 0;
        }

        /// <summary>
        /// check two include path and match based on the name of the file.
        /// </summary>
        /// <param name="path1"></param>
        /// <param name="path2"></param>
        /// <returns></returns>
        static bool IsMatchPath(string path1, string path2)
        {
            return path1.Split('/').Last() == path2.Split('/').Last();
        }

        /// <summary>
        /// if the content of file matched
        /// </summary>
        /// <param name="path1"></param>
        /// <param name="path2"></param>
        /// <returns></returns>
        static bool IsMatchContent(string path1, string path2)
        {
            return false;
        }

        /// <summary>
        /// returns the value associated with the variable in the Node
        /// </summary>
        /// <param name="node"></param>
        /// <param name="name"></param>
        /// <returns></returns>
        public static string NodeValue(Node node, string name)
        {
            node.Attributes.TryGetValue(name, out string attributeNameSource);
            return attributeNameSource;
        }


    }
    internal class GetAllNodesPostOrderVisitor : NodeVisitor<Node>
    {
        internal List<Node> Nodes { get; } = new List<Node>();

        public override Node VisitStruct(StructNode node)
        {
            node.Children.ForEach(c => c.AcceptVisitor(this));
            Nodes.Add(node);
            return node;
        }

        public override Node VisitSequence(SequenceNode node)
        {
            node.Children.ForEach(c => c.AcceptVisitor(this));
            Nodes.Add(node);
            return node;
        }
    }
}