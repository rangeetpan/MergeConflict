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
                    {
                        ret.Add(false);
                    }
                    else
                    {
                        ret.Add(true);
                    }
                }

                result[inputState] = ret.Cast<object>();
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        [WitnessFunction(nameof(Semantics.Apply), 1)]
        internal DisjunctiveExamplesSpec WitnessApplyAction(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
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
                        if (node.Attributes.TryGetValue("path", out string ret) && ret != string.Empty)
                        {
                            temp.Add(node);
                        }
                    }

                    specNode.Add(temp);
                }

                result[inputState] = specNode.AsReadOnly();
            }

            return DisjunctiveExamplesSpec.From(result);
        }

        /// <summary>
        ///     We take all the combinatiosn of the input tree that would produce the output tree.
        ///     E.g., output is Node1-> Node2-> Node3, then the input tree would be
        ///     (Node1, Node1->Node2, Node1->Node2->Node3)
        /// </summary>
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
                            {
                                temp.Add(prev);
                            }

                            temp.Add(output[i]);
                        }

                        possibleCombinations.Add(temp);
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
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                List<IReadOnlyList<Node>> possibleCombinations = new List<IReadOnlyList<Node>>();
                foreach (IReadOnlyList<Node> tree1NodeList in tree1Spec.DisjunctiveExamples[inputState])
                {
                    IEnumerable<Node> temp = from output in example.Value
                                             from outNode in (IReadOnlyList<Node>) output
                                             where tree1NodeList.All(
                                                 tree1Node => Semantics.NodeValue(tree1Node, "path") != Semantics.NodeValue(outNode, "path"))
                                             select outNode;
                    possibleCombinations.Add(temp.ToList());
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
                foreach (IReadOnlyList<Node> _ in example.Value)
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
                                outNode.Attributes.TryGetValue("path", out string outP);
                                n.Attributes.TryGetValue("path", out string inP);
                                if (inP == outP)
                                {
                                    flag = true;
                                }
                            }

                            if (!flag)
                            {
                                temp.Add(n);
                            }
                        }

                        possibleCombinations.Add(temp);
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
                result[inputState] = SelectIndex(example.Value, input.Upstream);
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
                result[inputState] = SelectIndex(example.Value, input.Downstream);
            }

            return DisjunctiveExamplesSpec.From(result);
        }

        private IEnumerable<object> SelectIndex(IEnumerable<object> list, IReadOnlyList<Node> element)
        {
            List<object> idx = new List<object>();
            foreach (IReadOnlyList<Node> output in list)
            {
                foreach (Node outputNode in output)
                {
                    outputNode.Attributes.TryGetValue("path", out string outputPath);
                    int count = 0;
                    foreach (Node node in element)
                    {
                        node.Attributes.TryGetValue("path", out string nodePath);
                        if (nodePath == outputPath)
                        {
                            idx.Add(count);
                        }

                        count++;
                    }
                }
            }

            return idx;
        }

        [WitnessFunction(nameof(Semantics.SelectDownstreamPath), 1)]
        internal DisjunctiveExamplesSpec WitnessSelectDownstreamPath(GrammarRule rule, DisjunctiveExamplesSpec spec)
        {
            var result = new Dictionary<State, IEnumerable<object>>();
            foreach (KeyValuePair<State, IEnumerable<object>> example in spec.DisjunctiveExamples)
            {
                State inputState = example.Key;
                var input = example.Key[Grammar.InputSymbol] as MergeConflict;
                result[inputState] = SelectPath(example.Value, input.Downstream);
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
                result[inputState] = SelectPath(example.Value, input.Upstream);
            }
            return DisjunctiveExamplesSpec.From(result);
        }

        private IEnumerable<object> SelectPath(IEnumerable<object> list, IReadOnlyList<Node> element)
        {
            List<object> paths = new List<object>();
            foreach (IReadOnlyList<Node> output in list)
            {
                foreach (Node outputNode in output)
                {
                    outputNode.Attributes.TryGetValue("path", out string outputPath);
                    foreach (Node node in element)
                    {
                        node.Attributes.TryGetValue("path", out string nodePath);
                        if (nodePath == outputPath)
                        {
                            paths.Add(nodePath);
                        }
                    }
                }
            }

            return paths;
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
                    {
                        idx.Add(i);
                    }
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
            var examples = task.Spec;
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
                        bool flag = false;
                        n.Attributes.TryGetValue("path", out string inPath);
                        foreach (Node node in output)
                        {
                            node.Attributes.TryGetValue("path", out string outputPath);
                            if (inPath == outputPath)
                            {
                                flag = true;
                            }
                        }
                        if (!flag)
                        {
                            idx.Add(inPath);
                        }
                    }
                }

                pathsArr.Add(idx.ToArray());
            }

            pathsArr.Add(new string[1] { string.Empty });
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