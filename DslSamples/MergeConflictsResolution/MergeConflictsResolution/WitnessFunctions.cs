using System.Collections.Generic;
using System.Linq;
using Microsoft.ProgramSynthesis;
using Microsoft.ProgramSynthesis.Learning;
using Microsoft.ProgramSynthesis.Rules;
using Microsoft.ProgramSynthesis.Specifications;
using Microsoft.ProgramSynthesis.Utils;
using Microsoft.ProgramSynthesis.Wrangling.Tree;
using Microsoft.ProgramSynthesis.VersionSpace;
using System.Threading;
using Microsoft.ProgramSynthesis.Learning.Strategies.Deductive.RuleLearners;
using Microsoft.ProgramSynthesis.AST;

namespace MergeConflictsResolution
{
    public partial class WitnessFunctions : DomainLearningLogic
    {
        public WitnessFunctions(Grammar grammar) : base(grammar) { }

        [WitnessFunction(nameof(Semantics.Apply), 0)]
        internal DisjunctiveExamplesSpec WitnessApplyPattern(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                List<bool> ret = new List<bool>();
                MergeConflict input = example.Key[Grammar.InputSymbol] as MergeConflict;
                foreach (IReadOnlyList<Node> output in example.Value)
                {
                    if (Semantics.Concat(input.Upstream, input.Downstream).Count == output.Count)
                        ret.Add(false);
                    else
                        ret.Add(true);
                }
                result[inputState] = ret.Cast<object>();
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [WitnessFunction(nameof(Semantics.Apply), 1)]
        internal DisjunctiveExamplesSpec WitnessApplyAction(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            string ret;
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                List<IReadOnlyList<Node>> specNode = new List<IReadOnlyList<Node>>();
                foreach (IReadOnlyList<Node> output in example.Value)
                {
                    List<Node> temp = new List<Node>();
                    foreach (Node node in output)
                    {
                        node.Attributes.TryGetValue("path", out ret);
                        if (ret != "")
                            temp.Add(node);
                    }
                    specNode.Add(temp.AsReadOnly());
                }
                result[inputState] = specNode.AsReadOnly();
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        /// <summary>
        /// Here, we are taking all the combination of the input tree that would produce the output
        /// tree, e.g., output is Node1-> Node2-> Node3, then the input tree would be
        /// (Node1, Node1->Node2, Node1->Node2->Node3)
        /// </summary>
        /// <param name="rule"></param>
        /// <param name="spec"></param>
        /// <returns></returns>
        [WitnessFunction(nameof(Semantics.Concat), 0)]
        internal DisjunctiveExamplesSpec WitnessConcatTree1(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                List<IReadOnlyList<Node>> possibleCombinations = new List<IReadOnlyList<Node>>();
                int count = 0;
                foreach (IReadOnlyList<Node> output in example.Value)
                {
                    for (var i = 0; i < output.Count - 1; i++)
                    {
                        List<Node> temp = new List<Node>();
                        if (count == 0)
                        {
                            temp.Add(output[i]);
                        }
                        else
                        {
                            IReadOnlyList<Node> previous = possibleCombinations[count - 1];
                            foreach (Node prev in previous)
                                temp.Add(prev);
                            temp.Add(output[i]);
                        }
                        possibleCombinations.Add(temp.AsReadOnly());
                        count++;
                    }
                }
                result[inputState] = possibleCombinations;
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [WitnessFunction(nameof(Semantics.Concat), 1, DependsOnParameters = new[] { 0 })]
        internal DisjunctiveExamplesSpec WitnessConcatTree2(GrammarRule rule, DisjunctiveExamplesSpec spec, DisjunctiveExamplesSpec tree1Spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            //string outP, inP;
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                List<IReadOnlyList<Node>> possibleCombinations = new List<IReadOnlyList<Node>>();
                foreach (IReadOnlyList<Node> tree1NodeList in tree1Spec.DisjunctiveExamples[inputState])
                {
                    List<Node> temp = new List<Node>();
                    foreach (IReadOnlyList<Node> output in example.Value)
                    {
                        foreach (Node outNode in output)
                        {
                            bool flag = false;
                            foreach (Node tree1Node in tree1NodeList)
                            {
                                if (Semantics.NodeValue(tree1Node, "path") == Semantics.NodeValue(outNode, "path"))
                                    flag = true;
                            }
                            if (flag == false)
                                temp.Add(outNode);
                        }
                    }
                    possibleCombinations.Add(temp.AsReadOnly());
                }
                result[inputState] = possibleCombinations;
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [WitnessFunction(nameof(Semantics.Remove), 0)]
        internal DisjunctiveExamplesSpec WitnessRemoveTree1(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                List<IReadOnlyList<Node>> possibleCombinations = new List<IReadOnlyList<Node>>();
                var input = example.Key[Grammar.InputSymbol] as MergeConflict;
                foreach (IReadOnlyList<Node> output in example.Value)
                {
                    possibleCombinations.Add(input.Upstream);
                    possibleCombinations.Add(input.Downstream);
                }
                result[inputState] = possibleCombinations;
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [WitnessFunction(nameof(Semantics.Remove), 1, DependsOnParameters = new[] { 0 })]
        internal DisjunctiveExamplesSpec WitnessRemoveTree2(GrammarRule rule, DisjunctiveExamplesSpec spec, DisjunctiveExamplesSpec tree1Spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            string outP, inP;
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                List<IReadOnlyList<Node>> possibleCombinations = new List<IReadOnlyList<Node>>();

                foreach (IReadOnlyList<Node> output in example.Value)
                {
                    foreach (IReadOnlyList<Node> tree1NodeList in tree1Spec.DisjunctiveExamples[inputState])
                    {
                        List<Node> temp = new List<Node>();
                        foreach (Node n in tree1NodeList)
                        {
                            bool flag = false;
                            foreach (Node outNode in output)
                            {
                                outNode.Attributes.TryGetValue("path", out outP);
                                n.Attributes.TryGetValue("path", out inP);
                                if (inP == outP)
                                    flag = true;
                            }
                            if (flag == false)
                                temp.Add(n);
                        }
                        possibleCombinations.Add(temp.AsReadOnly());
                    }
                }
                result[inputState] = possibleCombinations;
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [WitnessFunction(nameof(Semantics.Check), 1)]
        internal DisjunctiveExamplesSpec WitnessCheck(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                List<int[]> predicateInt = new List<int[]>();
                var duplicate = example.Key[rule.Body[0]] as List<IReadOnlyList<Node>>;
                List<int> temp = new List<int>();
                for (int i = 0; i < duplicate.Count; i++)
                {
                    if (duplicate[i].Count > 0)
                        temp.Add(i);
                }
                int[] tempArr = new int[temp.Count];
                int countInt = 0;
                foreach (int a in temp)
                {
                    tempArr[countInt] = a;
                    countInt++;
                }
                predicateInt.Add(tempArr);
                result[inputState] = predicateInt;
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [WitnessFunction(nameof(Semantics.SelectUpstreamIdx), 1)]
        internal DisjunctiveExamplesSpec WitnessSelectUpstream(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                var input = example.Key[Grammar.InputSymbol] as MergeConflict;
                List<int> idx = new List<int>();
                foreach (IReadOnlyList<Node> output in example.Value)
                {
                    foreach (Node node in output)
                    {
                        string outputPath;
                        node.Attributes.TryGetValue("path", out outputPath);
                        int count = 0;
                        foreach (Node upnode in input.Upstream)
                        {
                            string nodePath;
                            upnode.Attributes.TryGetValue("path", out nodePath);
                            if (nodePath == outputPath)
                                idx.Add(count);
                            count++;
                        }
                    }
                }
                result[inputState] = idx.Cast<object>();
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [WitnessFunction(nameof(Semantics.SelectDownstreamIdx), 1)]
        internal DisjunctiveExamplesSpec WitnessSelectDownstream(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                var input = example.Key[Grammar.InputSymbol] as MergeConflict;
                List<int> idx = new List<int>();
                foreach (IReadOnlyList<Node> output in example.Value)
                {
                    foreach (Node node in output)
                    {
                        string outputPath;
                        node.Attributes.TryGetValue("path", out outputPath);
                        int count = 0;
                        foreach (Node downnode in input.Downstream)
                        {
                            string nodePath;
                            downnode.Attributes.TryGetValue("path", out nodePath);
                            if (nodePath == outputPath)
                                idx.Add(count);
                            count++;
                        }
                    }
                }
                result[inputState] = idx.Cast<object>();
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [WitnessFunction(nameof(Semantics.SelectDownstreamPath), 1)]
        internal DisjunctiveExamplesSpec WitnessSelectDownstreamPath(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                var input = example.Key[Grammar.InputSymbol] as MergeConflict;
                List<string> idx = new List<string>();
                foreach (IReadOnlyList<Node> output in example.Value)
                {
                    foreach (Node node in output)
                    {
                        string outputPath;
                        node.Attributes.TryGetValue("path", out outputPath);
                        foreach (Node downnode in input.Downstream)
                        {
                            string nodePath;
                            downnode.Attributes.TryGetValue("path", out nodePath);
                            if (nodePath == outputPath)
                                idx.Add(nodePath);
                        }
                    }
                }
                result[inputState] = idx.Cast<object>();
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [WitnessFunction(nameof(Semantics.SelectUpstreamPath), 1)]
        internal DisjunctiveExamplesSpec WitnessSelectUpstreamPath(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                var input = example.Key[Grammar.InputSymbol] as MergeConflict;
                List<string> idx = new List<string>();
                foreach (IReadOnlyList<Node> output in example.Value)
                {
                    foreach (Node node in output)
                    {
                        string outputPath;
                        node.Attributes.TryGetValue("path", out outputPath);
                        foreach (Node upnode in input.Upstream)
                        {
                            string nodePath;
                            upnode.Attributes.TryGetValue("path", out nodePath);
                            if (nodePath == outputPath)
                                idx.Add(nodePath);
                        }
                    }
                }
                result[inputState] = idx.Cast<object>();
            }
            return DisjunctiveExamplesSpec.From(result);
        }
        [WitnessFunction(nameof(Semantics.SelectDup), 1)]
        internal DisjunctiveExamplesSpec WitnessSelectDup(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                var input = example.Key[rule.Body[0]] as List<IReadOnlyList<Node>>;
                List<int> idx = new List<int>();
                for (int i = 0; i < input.Count; i++)
                {
                    if (input[i].Count > 0)
                        idx.Add(i);
                }
                result[inputState] = idx.Cast<object>();
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [RuleLearner("dupLet")]
        internal Optional<ProgramSet> LearnDupLet(SynthesisEngine engine, LetRule rule,
                                                  LearningTask<DisjunctiveExamplesSpec> task,
                                                  CancellationToken cancel)
        {
            // TODO: compute path
            var examples = task.Spec as DisjunctiveExamplesSpec;
            List<string[]> pathsArr = new List<string[]>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in examples.DisjunctiveExamples)
            {
                State inputState = example.Key;
                var input = example.Key[Grammar.InputSymbol] as MergeConflict;
                List<string> idx = new List<string>();
                foreach (IReadOnlyList<Node> output in example.Value)
                {
                    foreach (Node n in Semantics.Concat(input.Upstream, input.Downstream))
                    {
                        string inPath;
                        bool flag = false;
                        n.Attributes.TryGetValue("path", out inPath);
                        foreach (Node node in output)
                        {
                            string outputPath;
                            node.Attributes.TryGetValue("path", out outputPath);
                            if (inPath == outputPath)
                                flag = true;
                        }
                        if (flag == false)
                            idx.Add(inPath);
                    }
                }
                string[] tempArr = new string[idx.Count];
                int countInt = 0;
                foreach (string a in idx)
                {
                    tempArr[countInt] = a;
                    countInt++;
                }
                pathsArr.Add(tempArr);
            }
            string[] temp = new string[1];
            temp[0] = "";
            pathsArr.Add(temp);
            List<ProgramSet> programSetList = new List<ProgramSet>();

            foreach (string[] path in pathsArr)
            {
                NonterminalRule findMatchRule = Grammar.Rule(nameof(Semantics.FindMatch)) as NonterminalRule;
                ProgramSet letValueSet =
                    ProgramSet.List(
                        Grammar.Symbol("find"),
                        new NonterminalNode(
                            findMatchRule,
                            new VariableNode(Grammar.InputSymbol),
                            new LiteralNode(Grammar.Symbol("paths"), path)));

                var bodySpec = new Dictionary<State, IEnumerable<object>>();
                foreach (KeyValuePair<State, IEnumerable<object>> kvp in task.Spec.DisjunctiveExamples)
                {
                    State input = kvp.Key;
                    MergeConflict x = (MergeConflict)input[Grammar.InputSymbol];
                    List<IReadOnlyList<Node>> dupValue = Semantics.FindMatch(x, path);

                    State newState = input.Bind(rule.Variable, dupValue);
                    bodySpec[newState] = kvp.Value;
                }

                LearningTask bodyTask = task.Clone(rule.LetBody, new DisjunctiveExamplesSpec(bodySpec));
                ProgramSet bodyProgramSet = engine.Learn(bodyTask, cancel);

                var dupLetProgramSet = ProgramSet.Join(rule, letValueSet, bodyProgramSet);
                programSetList.Add(dupLetProgramSet);
            }

            ProgramSet ps = new UnionProgramSet(rule.Head, programSetList.ToArray());
            return ps.Some();
        }
    }
}