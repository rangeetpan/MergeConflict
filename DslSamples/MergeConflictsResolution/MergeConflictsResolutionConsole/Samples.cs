using System;
using System.Collections.Generic;
using MergeConflictsResolution;

namespace MergeConflictsResolutionConsole
{
    class Samples
    {
        static void Main(string[] args)
        {
            // Example 1c 1d
            string conflict1 = @"<<<<<<< HEAD
#include ""base/command_line.h""
#include ""base/logging.h""
=======
#include ""base/check_op.h""
>>>>>>>";
            string resolution1 = @"#include ""base/command_line.h""
#include ""base/check_op.h""";

            string conflict2 = @"<<<<<<< HEAD
#include ""base/logging.h""
#include ""base/scoped_native_library.h""
=======
#include ""base/notreached.h""
>>>>>>>";
            string resolution2 = @"#include ""base/scoped_native_library.h""
#include ""base/notreached.h""";

            MergeConflict input1 = new MergeConflict(conflict1);
            MergeConflict input2 = new MergeConflict(conflict2);
            List<ResolutionExample> examples = new List<ResolutionExample>
            {
                new ResolutionExample(input1, resolution1),
                new ResolutionExample(input2, resolution2)
            };

            Program program = Learner.Instance.Learn(examples);
            string resolution = program.RunString(input1);
            Console.WriteLine(resolution);
        }
    }
}
