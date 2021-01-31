using System.Collections.Generic;
using System.Text.RegularExpressions;
using Microsoft.ProgramSynthesis;
using Microsoft.ProgramSynthesis.AST;
using Microsoft.ProgramSynthesis.Features;
using Microsoft.ProgramSynthesis.Utils;

namespace ProseTutorial
{
    public class RankingScore : Feature<double>
    {
        public RankingScore(Grammar grammar) : base(grammar, "Score") { }

        protected override double GetFeatureValueForVariable(VariableNode variable) => 0;

        [FeatureCalculator(nameof(Semantics.Apply))]
        public static double ApplyScore(double pattern, double action) => pattern + action;

        [FeatureCalculator(nameof(Semantics.Remove))]
#pragma warning disable IDE1006 // Naming Styles
        public static double removeScore(double input, double selected) => input;

        [FeatureCalculator(nameof(Semantics.Concat))]
        public static double concatScore(double input1, double input2) => input1 + input2;
        //public static double concatScore(double input1, double input2) => input1 > input2 ? input1 + 2 * input2 : 2 * input1 + input2; 
        [FeatureCalculator(nameof(Semantics.SelectUpstreamIdx))]
        public static double Select_upstream_idxScore(double x, double k) => x + k - 20;

        [FeatureCalculator(nameof(Semantics.SelectDownstreamIdx))]
        public static double Select_downstream_idxScore(double x, double k) => x + k - 20;

        [FeatureCalculator(nameof(Semantics.SelectUpstream))]
        public static double Select_upstreamScore(double x) => x;

        [FeatureCalculator(nameof(Semantics.SelectDownstream))]
        public static double Select_downstreamScore(double x) => x;

        [FeatureCalculator(nameof(Semantics.SelectUpstreamPath))]
        public static double Select_upstream_pathScore(double x, double k) => x + k;

        [FeatureCalculator(nameof(Semantics.SelectDownstreamPath))]
        public static double Select_downstream_pathScore(double x, double k) => x + k;

        [FeatureCalculator(nameof(Semantics.FindMatch))]
        public static double findMatchScore(double x, double k) => x + k;

        [FeatureCalculator(nameof(Semantics.Check))]
        public static double checkScore(double x, double k) => x + k;

        [FeatureCalculator(nameof(Semantics.SelectDup))]
        public static double checkSelectDup(double x, double k) => x + k;

        [FeatureCalculator("path", Method = CalculationMethod.FromLiteral)]
        public static double Score_Path(string path) => 0;

        [FeatureCalculator("enabledPredicate", Method = CalculationMethod.FromLiteral)]
        public static double Score_enabledPredicate(int[] enabledPredicate) => 0;

        [FeatureCalculator("paths", Method = CalculationMethod.FromLiteral)]
        public static double Score_paths(string[] paths) => 0;

        [FeatureCalculator("k", Method = CalculationMethod.FromLiteral)]
        public static double Score_k(int k) => 0;
    }
}