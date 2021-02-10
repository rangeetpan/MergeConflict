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

            string conflict3 = @"<<<<<<< HEAD
#include ""ui/base/anonymous_ui_base_features.h""
#include ""base/logging.h""
=======
#include ""ui/base/cursor/mojom/cursor_type.mojom-blink.h""
>>>>>>>";
            string resolution2 = @"#include ""base/scoped_native_library.h""
#include ""base/notreached.h""";

            MergeConflict input1 = new MergeConflict(conflict1);
            MergeConflict input2 = new MergeConflict(conflict2);
            MergeConflict input3 = new MergeConflict(conflict3);
            List<ResolutionExample> examples = new List<ResolutionExample>
            {
                new ResolutionExample(input1, resolution1),
                new ResolutionExample(input2, resolution2)
            };

            // Learn with two examples
            Program program = Learner.Instance.Learn(examples);

            // Execute the program on the input to obtained the resolution
            // TODO: create a new input (don't use input 1)
            string resolution = program.RunString(input3);
            Console.WriteLine(resolution);
        }
    }
}
