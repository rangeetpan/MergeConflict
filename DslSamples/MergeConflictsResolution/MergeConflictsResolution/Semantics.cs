using System.Collections.Generic;
using System.Linq;
using Microsoft.ProgramSynthesis.Utils;
using Microsoft.ProgramSynthesis.Utils.Interactive;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace MergeConflictsResolution
{
    /// <summary>
    /// The implementations of the operators in the MergeConflictsResolution language.
    /// </summary>
    public static class Semantics
    {
        private static string[] upstream_specific_keywords = { "chrome", "google" };
        private static string[] downstream_specific_keywords = { "edge", "microsoft", "EDGE" };

        public static IReadOnlyList<Node> Apply(bool pattern, IReadOnlyList<Node> action)
        {
            if (pattern == true)
            {
                return action;
            }

            return null;
        }

        public static IReadOnlyList<Node> Remove(IReadOnlyList<Node> input, IReadOnlyList<Node> selected)
        {
            List<Node> input_clone = input.ToList();
            List<Node> selectedNode = selected.ToList();
            List<int> index = new List<int>();
            int returnIndex;
            foreach (Node n in selectedNode)
            {
                returnIndex = IndexNode(input, n);
                if (returnIndex != -1)
                    index.Add(returnIndex);
            }

            foreach (int indice in index.OrderByDescending(v => v))
            {
                input_clone.RemoveAt(indice);
            }

            return input_clone.AsReadOnly();
        }

        public static int IndexNode(IReadOnlyList<Node> input, Node selected)
        {
            string attrValue = NodeValue(selected, "path");
            int k = 0;
            foreach (Node n in input)
            {
                if (NodeValue(n, "path") == attrValue)
                    return k;
                k++;
            }
            return -1;
        }

        public static IReadOnlyList<Node> Concat(IReadOnlyList<Node> input1, IReadOnlyList<Node> input2)
        {
            //temp = input1;
            //input1.concat(input2);
            List<Node> temp = input1.ToList();
            foreach (Node n in input2)
            {
                temp.Add(n);
            }
            return temp.AsReadOnly();
        }

        /// <summary>
        /// Selects the upstream line by either index.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="k"></param>
        /// <returns></returns>
        /// Write own class 
        public static IReadOnlyList<Node> SelectUpstreamIdx(MergeConflict x, int k)
        {
            int count = 0;
            List<Node> temp = new List<Node>();
            foreach (Node upstream in x.Upstream)
            {
                if (count == k)
                    temp.Add(upstream);
                count = count + 1;
            }
            return temp.AsReadOnly();
        }

        public static IReadOnlyList<Node> AllNodes(Node node)
        {
            var visitor = new GetAllNodesPostOrderVisitor();
            node.AcceptVisitor(visitor);
            return visitor.Nodes.ToArray();
        }
        /// <summary>
        /// select the downstream line by either index 
        /// </summary>
        /// <param name="x"></param>
        /// <param name="k"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectDownstreamIdx(MergeConflict x, int k)
        {
            int count = 0;
            List<Node> temp = new List<Node>();
            foreach (Node downstream in x.Downstream)
            {
                if (count == k)
                    temp.Add(downstream);
                count = count + 1;
            }
            return temp.AsReadOnly();
        }
        /// <summary>
        /// Select downstream
        /// </summary>
        /// <param name="x"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectDownstream(MergeConflict x)
        {
            return x.Downstream;
        }
        /// <summary>
        /// Select upstream
        /// </summary>
        /// <param name="x"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectUpstream(MergeConflict x)
        {
            return x.Upstream;
        }
        /// <summary>
        /// Select the node with the specified path (upstream)
        /// </summary>
        /// <param name="x"></param>
        /// <param name="k"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectDownstreamPath(MergeConflict x, string k)
        {
            string ret;
            List<Node> temp = new List<Node>();
            foreach (Node downstream in x.Downstream)
            {
                downstream.Attributes.TryGetValue("path", out ret);
                if (ret == k)
                    temp.Add(downstream);
            }
            return temp.AsReadOnly();
        }
        /// <summary>
        /// Select the node with the specified path (downstream)
        /// </summary>
        /// <param name="x"></param>
        /// <param name="k"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectUpstreamPath(MergeConflict x, string k)
        {
            string ret;
            List<Node> temp = new List<Node>();
            foreach (Node upstream in x.Upstream)
            {
                upstream.Attributes.TryGetValue("path", out ret);
                if (ret == k)
                    temp.Add(upstream);
            }
            return temp.AsReadOnly();
        }
        public static List<IReadOnlyList<Node>> FindMatch(MergeConflict x, string[] paths)
        {
            List<IReadOnlyList<Node>> returnNode = new List<IReadOnlyList<Node>>();
            returnNode.Add(FindDuplicateInUpstreamAndDownstream(x));
            returnNode.Add(FindFreqPattern(x, paths));
            returnNode.Add(FindDuplicateInDownstreamOutside(x));
            returnNode.Add(FindDuplicateInUpstreamOutside(x));
            returnNode.Add(FindUpstreamSpecific(x));
            returnNode.Add(FindDownstreamSpecific(x));
            return returnNode;
        }

        public static bool Check(List<IReadOnlyList<Node>> dub, int[] enabledPredicate)
        {
            bool checkFlag = true;
            foreach (int predicateCheck in enabledPredicate)
            {
                if (dub[predicateCheck].Count > 0)
                    checkFlag = checkFlag & true;
                else
                    checkFlag = checkFlag & false;
            }
            return checkFlag;
        }

        public static IReadOnlyList<Node> SelectDup(List<IReadOnlyList<Node>> dup, int k)
        {
            return dup[k];
        }
        /// <summary>
        /// Identify duplicate headers inside the conflicting region
        /// </summary>
        /// <param name="x"></param>
        /// <returns></returns>
        public static List<Node> FindDuplicateInUpstreamAndDownstream(MergeConflict x)
        {
            List<Node> nodes = new List<Node>();
            foreach (Node upstream in x.Upstream)
            {
                foreach (Node downstream in x.Downstream)
                {
                    if (IsmatchPath(NodeValue(upstream, "path"), NodeValue(downstream, "path")))
                        nodes.Add(upstream);
                    else
                    {
                        if (IsmatchContent(NodeValue(upstream, "path"), NodeValue(downstream, "path")))
                        {
                            nodes.Add(upstream);
                        }
                    }

                }
            }
            return nodes;
        }

        public static IReadOnlyList<Node> FindFreqPattern(MergeConflict x, string[] paths)
        {
            List<Node> nodes = new List<Node>();
            foreach (Node stream in Concat(x.Downstream, x.Upstream))
            {
                foreach (string path in paths)
                {
                    if (NodeValue(stream, "path") == path)
                        nodes.Add(stream);
                }
            }
            return nodes;
        }
        /// <summary>
        /// Identify duplicate headers outside the conflicting region (downstream specific)
        /// </summary>
        /// <param name="x"></param>
        /// <returns></returns>
        public static List<Node> FindDuplicateInDownstreamOutside(MergeConflict x)
        {
            List<Node> nodes = new List<Node>();
            //downstream_file_include_AST = file_to_AST(x.downstream_content);
            IReadOnlyList<Node> downstream_file_include_AST = x.UpstreamContent;
            foreach (Node downstream in x.Downstream)
            {
                foreach (Node outside_include in downstream_file_include_AST)
                {
                    if (IsmatchPath(NodeValue(outside_include, "path"), NodeValue(downstream, "path")))
                        nodes.Add(downstream);
                    else
                    {
                        if (IsmatchContent(NodeValue(outside_include, "path"), NodeValue(downstream, "path")))
                        {
                            nodes.Add(downstream);
                        }
                    }
                }
            }
            return nodes;
        }
        /// <summary>
        /// Identify duplicate headers outside the conflicting region (upstream specific)
        /// </summary>
        /// <param name="x"></param>
        /// <returns></returns>
        public static List<Node> FindDuplicateInUpstreamOutside(MergeConflict x)
        {
            List<Node> nodes = new List<Node>();
            //upstream_file_include_AST = file_to_AST(x.upstream_content);
            IReadOnlyList<Node> upstream_file_include_AST = x.UpstreamContent;
            foreach (Node upstream in x.Upstream)
            {
                foreach (Node outside_include in upstream_file_include_AST)
                {
                    if (IsmatchPath(NodeValue(outside_include, "path"), NodeValue(upstream, "path")))
                        nodes.Add(upstream);
                    else
                    {
                        if (IsmatchContent(NodeValue(outside_include, "path"), NodeValue(upstream, "path")))
                        {
                            nodes.Add(upstream);
                        }
                    }
                }
            }
            return nodes;
        }
        /// <summary>
        /// identify the upstream specific header path
        /// </summary>
        /// <param name="x"></param>
        /// <returns></returns>
        public static List<Node> FindUpstreamSpecific(MergeConflict x)
        {
            List<Node> nodes = new List<Node>();
            foreach (Node upstream in x.Upstream)
            {
                if (!NodeValue(upstream, "path").Contains(".h"))
                {
                    bool flag = false;
                    foreach (Node downstream in x.Downstream)
                    {
                        if (NodeValue(upstream, "path").Split('(')[0] != NodeValue(downstream, "path").Split('(')[0])
                        {
                            flag = true;
                        }
                    }
                    if (flag == true)
                    {
                        if (downstream_specific_keywords.Any(s => NodeValue(upstream, "path").Contains(s)))
                            nodes.Add(upstream);
                    }
                }
                else if (downstream_specific_keywords.Any(s => NodeValue(upstream, "path").Contains(s)))
                    nodes.Add(upstream);
            }
            return nodes;
        }
        /// <summary>
        /// identify the downstream specific header path 
        /// </summary>
        /// <param name="x"></param>
        /// <returns></returns>
        public static List<Node> FindDownstreamSpecific(MergeConflict x)
        {
            List<Node> nodes = new List<Node>();
            foreach (Node downstream in x.Downstream)
            {
                if (!NodeValue(downstream, "path").Contains(".h"))
                {
                    bool flag = false;
                    foreach (Node upstream in x.Upstream)
                    {
                        if (NodeValue(downstream, "path").Split('(')[0] == NodeValue(upstream, "path").Split('(')[0])
                        {
                            flag = true;
                        }
                    }
                    if (flag == true)
                    {
                        if (downstream_specific_keywords.Any(s => NodeValue(downstream, "path").Contains(s)))
                            nodes.Add(downstream);
                    }
                }
                else if (downstream_specific_keywords.Any(s => NodeValue(downstream, "path").Contains(s)))
                    nodes.Add(downstream);
            }
            return nodes;
        }

        //public static List<Node> FindDependencyDownstream(MergeConflict x) {
        //    throw new NotImplementedException();
        //}

        //public static List<Node> FindDependencyUpstream(MergeConflict x) {
        //    throw new NotImplementedException();
        //}

        public static bool Match(IReadOnlyList<Node> l)
        {
            if (l.Count == 0)
            {
                return false;
            }
            return true;
        }

        static bool IsmatchPath(string path1, string path2)
        {
            string[] path1_split, path2_split;
            path1_split = path1.Split('/');
            path2_split = path2.Split('/');
            if (path1_split[path1_split.Length - 1] == path2_split[path2_split.Length - 1])
                return true;
            return false;
        }

        static bool IsmatchContent(string path1, string path2)
        {
            return false;
        }

        static bool Match(Node predicate)
        {
            if (predicate == null)
                return false;
            return true;
        }
        public static string NodeValue(Node node, string name)
        {
            string attributeNameSource;
            node.Attributes.TryGetValue(name, out attributeNameSource);
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