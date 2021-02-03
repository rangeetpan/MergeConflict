using System.Reflection;
using Microsoft.ProgramSynthesis;
using Microsoft.ProgramSynthesis.Compiler;
using Microsoft.ProgramSynthesis.Wrangling.Tree;
namespace MergeConflictsResolution
{
    public class LanguageGrammar
    {
        private const string GrammarContent = @"
using Microsoft.ProgramSynthesis.Wrangling.Tree;
using System.Collections.Generic;
using MergeConflictsResolution;

using semantics MergeConflictsResolution.Semantics;
using learners MergeConflictsResolution.WitnessFunctions;

language MergeConflictsResolution;

@complete feature double Score = RankingScore;

@input MergeConflict x;
string path;
string [] paths;
int k;

int[] enabledPredicate;

@start IReadOnlyList<Node> proc:= rule;

IReadOnlyList<Node> rule := @id['dupLet'] let dup: List<IReadOnlyList<Node>> = find in Apply(predicate, action);

bool predicate := Check(dup, enabledPredicate);

IReadOnlyList<Node> action  := Concat(action, action)
							|  Remove(t, t)
							|  t;

IReadOnlyList<Node> t       := SelectUpstreamIdx(x, k)
                            |  SelectUpstreamPath(x, path)
                            |  SelectDownstreamIdx(x, k)
                            |  SelectDownstreamPath(x, path)
                            |  SelectDownstream(x)
                            |  SelectUpstream(x)
							|  SelectDup(dup, k);

List<IReadOnlyList<Node>> find	:= FindMatch(x, paths);";

        private LanguageGrammar()
        {
            System.Collections.Generic.IReadOnlyList<CompilerReference> References = CompilerReference.FromAssemblyFiles(typeof(Semantics).GetTypeInfo().Assembly);
            var compile = DSLCompiler.Compile(new CompilerOptions
            {
                InputGrammarText = GrammarContent,
                References = CompilerReference.FromAssemblyFiles(typeof(Semantics).GetTypeInfo().Assembly)
            });
            Grammar = compile.Value;
        }

        /// <summary>
        ///     Singleton instance of <see cref="LanguageGrammar" />.
        /// </summary>
        public static LanguageGrammar Instance { get; } = new LanguageGrammar();

        public Grammar Grammar { get; private set; }
    }
}
