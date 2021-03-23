using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Microsoft.ProgramSynthesis.Utils;
using Microsoft.ProgramSynthesis.Utils.Interactive;
using Microsoft.ProgramSynthesis.Wrangling.Tree;

namespace Microsoft.ProgramSynthesis.Transformation.Conflict.Semantics {
    /// <summary>
    /// The implementations of the operators in the Transformation.Conflict language.
    /// </summary>
    public static class Semantics {
#pragma warning disable IDE1006 // Naming Styles
        private static string[] upstream_specific_keywords = { "chrome", "google" };
        private static string[] downstream_specific_keywords = { "edge", "microsoft","EDGE" };
        //private static string path_separator="//";

        public static IReadOnlyList<Node> Apply(bool pattern, IReadOnlyList<Node> action)
        {
            if(pattern ==true)
            {
                return action;
            }
            return null;
        }
        public static IReadOnlyList<Node> Remove(IReadOnlyList<Node> input, IReadOnlyList<Node> selected) {
            List<Node> input_clone = input.ToList();
            List<Node> selectedNode = selected.ToList();
            List<int> index = new List<int>();
            int returnIndex;
            foreach (Node n in selectedNode) {
                returnIndex = IndexNode(input, n);
                if (returnIndex != -1)
                    index.Add(returnIndex);
            }
            foreach (int indice in index.OrderByDescending(v => v)) {
                input_clone.RemoveAt(indice);
            }
            return input_clone.AsReadOnly();
        }
        public static int IndexNode(IReadOnlyList<Node> input, Node selected) {
            string attrValue = Nodevalue(selected, "path");
            int k = 0;
            foreach(Node n in input) {
                if (Nodevalue(n, "path") == attrValue)
                    return k;
                k = k + 1;
            }
            return -1;
        }
        //IReadOnlyList
        public static IReadOnlyList<Node> Concat(IReadOnlyList<Node> input1, IReadOnlyList<Node> input2) {
            //temp = input1;
            //input1.concat(input2);
            List<Node> temp = input1.ToList();
            foreach(Node n in input2) {
                temp.Add(n);
            }
            return temp.AsReadOnly();
        }

        /// <summary>
        /// select the upstream line by either index 
        /// </summary>
        /// <param name="x"></param>
        /// <param name="k"></param>
        /// <returns></returns>
        /// Write own class 
        public static IReadOnlyList<Node> SelectUpstreamIdx(MergeConflict x, int k)
        {
            int count = 0;
            List<Node> temp = new List<Node>();
            foreach (Node upstream in x.upstream)
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
            foreach (Node downstream in x.downstream) {
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
            return x.downstream;
        }
        /// <summary>
        /// Select upstream
        /// </summary>
        /// <param name="x"></param>
        /// <returns></returns>
        public static IReadOnlyList<Node> SelectUpstream(MergeConflict x)
        {
            return x.upstream;
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
            foreach (Node downstream in x.downstream)
            {
                downstream.Attributes.TryGetValue("path", out ret);
                if (ret== k)
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
            foreach (Node upstream in x.upstream)
            {
                upstream.Attributes.TryGetValue("path", out ret);
                if (ret == k)
                    temp.Add(upstream);
            }
            return temp.AsReadOnly();
        }
        public static List<IReadOnlyList<Node>> FindMatch(MergeConflict x, string[] paths) {
            List<IReadOnlyList<Node>> returnNode = new List<IReadOnlyList<Node>>();
            returnNode.Add(FindDuplicateInUpstreamAndDownstream(x));
            returnNode.Add(FindFreqPattern(x, paths));
            returnNode.Add(FindDuplicateInDownstreamOutside(x));
            returnNode.Add(FindDuplicateInUpstreamOutside(x));
            returnNode.Add(FindUpstreamSpecific(x));
            returnNode.Add(FindDownstreamSpecific(x));
            returnNode.Add(FindDuplicateDownstreamSpecific(x));
            returnNode.Add(FindDependency(x));

            return returnNode;
        }
        public static IReadOnlyList<Node> FindDuplicateDownstreamSpecific(MergeConflict x) {
            List<Node> upstream = new List<Node>();
            List<Node> downstream = new List<Node>();
            foreach (Node n in x.upstream) {
                if (upstream_specific_keywords.Any(s => Nodevalue(n, "path").Contains(s)))
                    upstream.Add(n);
            }
            foreach (Node n in x.downstream) {
                if (downstream_specific_keywords.Any(s => Nodevalue(n, "path").Contains(s)))
                    downstream.Add(n);
            }
            
            List<Node> temp = new List<Node>();
            foreach(Node up in upstream) {
                foreach(Node down in downstream){
                    string downP = "";
                    string upP = "";
                    string[] uppath = Nodevalue(up, "path").Split('_');
                    string[] downpath = Nodevalue(down, "path").Split('_');
                    for(int i=1; i<uppath.Length; i++) {
                        upP = upP + uppath[i];
                    }
                    for (int i = 1; i < downpath.Length; i++) {
                        downP = downP + downpath[i];
                    }
                    if (upP == downP)
                        temp.Add(up);
                }
            }
            return temp.AsReadOnly();
        }
        public static List<string> conflict_content(MergeConflict x) {
            string Head_lookup = "<<<<<<< HEAD";
            string End_lookup = ">>>>>>>";
            bool flag = false;
            List<string> lines = new List<string>();
            string text = "";
            int count = 0;
            foreach (string line in x.string_content) {
                count = count + 1;
                if (flag == true) {
                    if (line.Contains(End_lookup)) {
                        flag = false;
                        lines.Add(text);
                    }
                    else {
                        if (line != "=======")
                            text = text + line.Replace("\n", "").Replace("\r", "");
                    }
                }
                if (line.Contains(Head_lookup)) {
                        flag = true;
                }
            }
            return lines;
        }
        public static List<string> non_conflict_content(MergeConflict x) {
            string Head_lookup = "<<<<<<< HEAD";
            string End_lookup = ">>>>>>>";
            bool flag = false;
            List<string> lines = new List<string>();
            int count = 0;
            foreach (string line in x.string_content) {
                count = count + 1;
                if(flag==false)
                    lines.Add(line);
                if (flag == true) {
                    if (line.Contains(End_lookup)) {
                        flag = false;
                        
                    }
                }
                if (line.Contains(Head_lookup)) {
                    flag = true;
                }
            }
            return lines;
        }
        public static IReadOnlyList<Node> FindDependency(MergeConflict x) {
            List<Node> temp = new List<Node>();
            List<string> conflicts = conflict_content(x);
            List<string> non_conflicts = non_conflict_content(x);
            foreach (Node n in Semantics.Concat(x.upstream, x.downstream)) {
                string filePath = x.basePath + "\\" + Nodevalue(n, "path").Replace("/", "\\");
                if (File.Exists(@filePath)) { 
                string[] lines = System.IO.File.ReadAllLines(@filePath);
                bool flag_conflict = false;
                bool flag_non_conflict = false;
                bool flag = false;
                foreach (string line in lines) {
                    if (!line.StartsWith("//") && flag == false) {
                        if (line.Contains("namespace")) {
                            string[] words = line.Split(' ');
                            int index = 0;
                            for (int i = 0; i < words.Length; i++) {
                                if (words[i] == "namespace") {
                                    index = i;
                                    flag = true;
                                }
                            }
                            for (int i = index + 1; i < words.Length; i++) {
                                if (words[i] != "{" && words[i] != " ") {
                                    //foreach(Node otherConflict in x.other_conflict_content) {
                                    //    if (Nodevalue(otherConflict, "path").Contains(words[i]+"::"))
                                    //        temp.Add(n);
                                    //}
                                    foreach (string conflict in conflicts) {
                                        if (conflict.Contains(words[i] + "::"))
                                            flag_conflict = true;
                                    }
                                    foreach (string conflict in non_conflicts) {
                                        if (conflict.Contains(words[i] + "::"))
                                            flag_non_conflict = true;
                                    }
                                }
                            }
                            if (flag_conflict == true && flag_non_conflict == false)
                                temp.Add(n);

                        }
                    }
                }
            }
            }
            return temp.AsReadOnly();           
        }
        public static bool Check(List<IReadOnlyList<Node>> dub, int[] enabledPredicate) {
            bool checkFlag = true;
            foreach(int predicateCheck in enabledPredicate) {
                if (dub[predicateCheck].Count > 0)
                    checkFlag = checkFlag & true;
                else
                    checkFlag = checkFlag & false;
            }
            return checkFlag;
        }

        public static IReadOnlyList<Node> SelectDup(List<IReadOnlyList<Node>> dup, int k) {
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
            foreach (Node upstream in x.upstream)
            {
                foreach(Node downstream in x.downstream)
                {
                    if (IsmatchPath(Nodevalue(upstream, "path"), Nodevalue(downstream, "path")))
                        nodes.Add(upstream);
                    else
                    {
                        if (IsmatchContent(Nodevalue(upstream, "path"), Nodevalue(downstream, "path")))
                        {
                            nodes.Add(upstream);
                        }
                    }

                }
            }
            return nodes;
        }

        public static IReadOnlyList<Node> FindFreqPattern(MergeConflict x, string[] paths) {
            List<Node> nodes = new List<Node>();
            foreach (Node stream in Concat(x.downstream, x.upstream)) {
                foreach(string path in paths) {
                    if (Nodevalue(stream, "path") == path)
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
            IReadOnlyList<Node> downstream_file_include_AST = x.path_content;
            foreach (Node downstream in x.downstream)
            {
                bool flag = false;
                foreach (Node outside_include in downstream_file_include_AST) {
                    if (flag == false) 
                    { 
                        if (IsmatchPath(Nodevalue(outside_include, "path"), Nodevalue(downstream, "path"))) {
                            nodes.Add(downstream);
                            flag = true;
                        }
                        else {
                            if (IsmatchContent(Nodevalue(outside_include, "path"), Nodevalue(downstream, "path"))) {
                                nodes.Add(downstream);
                                flag = true;
                            }
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
            IReadOnlyList<Node> upstream_file_include_AST = x.path_content;
            foreach (Node upstream in x.upstream)
            {
                bool flag = false;
                foreach (Node outside_include in upstream_file_include_AST)
                {
                    if (flag == false) {
                        if (IsmatchPath(Nodevalue(outside_include, "path"), Nodevalue(upstream, "path"))) {
                            nodes.Add(upstream);
                            flag = true;
                        }
                        else {
                            if (IsmatchContent(Nodevalue(outside_include, "path"), Nodevalue(upstream, "path"))) {
                                nodes.Add(upstream);
                                flag = true;
                            }
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
            foreach (Node upstream in x.upstream) {
                if (!Nodevalue(upstream, "path").Contains(".h")) {
                    bool flag = false;
                    foreach (Node downstream in x.downstream) {
                        if (Nodevalue(upstream, "path").Split('(')[0] != Nodevalue(downstream, "path").Split('(')[0]) {
                            flag = true;
                        }
                    }
                    if (flag == true) {
                        if (downstream_specific_keywords.Any(s => Nodevalue(upstream, "path").Contains(s)))
                            nodes.Add(upstream);
                    }
                }
                else if (downstream_specific_keywords.Any(s => Nodevalue(upstream, "path").Contains(s)))
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
            foreach (Node downstream in x.downstream)
            {
                if(!Nodevalue(downstream, "path").Contains(".h")) {
                    bool flag = false;
                    foreach(Node upstream in x.upstream) {
                        if(Nodevalue(downstream, "path").Split('(')[0]== Nodevalue(upstream, "path").Split('(')[0]) {
                            flag = true;
                        }
                    }
                    if(flag==true) {
                        if (downstream_specific_keywords.Any(s => Nodevalue(downstream, "path").Contains(s)))
                            nodes.Add(downstream);
                    }
                }
                else if (downstream_specific_keywords.Any(s => Nodevalue(downstream, "path").Contains(s)))
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

        public static bool Match(IReadOnlyList<Node> l) {
            if(l.Count==0) {
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
        public static string Nodevalue(Node node, string name) {
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
