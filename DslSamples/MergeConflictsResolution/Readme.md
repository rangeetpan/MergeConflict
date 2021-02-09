This project contains the PROSE framework for the International Conference on Software Engineering (ICSE'21) paper on **Can Program Synthesis be Used to Learn Merge Conflict Resolutions? An Empirical Analysis**. This work has been done by [Rangeet Pan](https://rangeetpan.github.io/), [Vu Le](https://www.microsoft.com/en-us/research/people/levu/), [Nachiappan Nagappan](https://nachinagappan.github.io/), [Sumit Gulwani](https://www.microsoft.com/en-us/research/people/sumitg/), [Shuvendu Lahiri](https://www.microsoft.com/en-us/research/people/shuvendu/), and [Mike Kaufman](https://www.linkedin.com/in/mike-kaufman-439622/).

Abstract: In this paper, we study the problem of textual merge conflicts from the perspective of Microsoft Edge, a large, highly collaborative fork off the main Chromium branch with significant merge conflicts. 
Broadly, this study is divided into two sections. First, we empirically evaluate textual merge conflicts in Microsoft Edge and classify them based on the type of files, location of conflicts in a file, and the size of conflicts.
We found that ~28% of the merge conflicts are 1-2 line changes, and many resolutions have frequent patterns. 
Second, driven by these findings, we explore Program Synthesis (for the first time) to learn patterns and resolve structural merge conflicts. 
We propose a novel domain-specific language (DSL) that captures many of the repetitive merge conflict resolution patterns and learn resolution strategies as programs in this DSL from example resolutions. We found that the learned strategies can resolve 11.4% of the conflicts (~41% of 1-2 line changes) that arise in the  C++ files with 93.2% accuracy.

In this tutorial, we will learn how to synthesize programs for learning string transformations.

## Set up (Windows).

If you get stuck at any of the steps in this section, please send an email to Vu Le at levu@microsoft.com and Rangeet Pan at rangeet@iastate.edu.

1. Get Visual Studio.

+ Option 1: Download Visual Studio 2017 Community: https://www.visualstudio.com/downloads/ (Be sure to select the ".Net Desktop Development" box in the installation wizard).

+ Option 2: Install the windows VM from this Official Microsoft link: https://developer.microsoft.com/en-us/windows/downloads/virtual-machines. This VM comes with VS installed. When you open VS 2017, it may say that your subscription is expired. Just sign in with your account (or sign up for free).

+ Option 3: Install Visual Studio Code (See instructions in the Linux and Mac section)

2. Install .NET Core SDK (https://www.microsoft.com/net/download/)

3. Clone this repository:

    ```
    git clone https://github.com/Microsoft/prose.git
    cd prose\DslAuthoringTutorial\part1a
    ```

4. Open the project solution (prose.sln) in Visual Studio.

5. Build the project: Solution Explorer -> Right-Click on Solution 'ProseTutorial' -> Build

## Set up (Linux and Mac).

If you get stuck at any of the steps in this section, please send an email to Gustavo Soares at gsoares@microsoft.com and Sumit Gulwani at sumitg@microsoft.com.

1.  Download and install VS Code (https://code.visualstudio.com/Download)

1.  Install .NET Core SDK (https://www.microsoft.com/net/download/linux)

1.  Install VS Code extension for C#. (https://marketplace.visualstudio.com/items?itemName=ms-vscode.csharp)

1.  Clone this repository:

    ```
    git clone https://github.com/Microsoft/prose.git
    cd prose/DslAuthoringTutorial/part1a
    ```

1.  Open this folder in VS Code:

    +   Open VS Code

    +   Choose “Open Folder” and select the “ICSE'21_Merge_Conflict” folder.

    +   VS Code may show a warning at the top of the screen about missing dependences. Click on “yes” to install them.

1.  Building the project:

    +   Open the VS Code terminal: View -> Integrated Terminal (or Ctrl+\`)

        ```
        dotnet build
        ```

        If the project was built successfully, a message confirming that should be printed:

        ```
        Build succeeded.
            0 Warning(s)
            0 Error(s)
        ```

1.  Running the tests:

    +   Open the VS Code terminal: View -> Integrated Terminal (or Ctrl+\`)

        ```
        dotnet test
        ```

        All 2 tests should pass:

        ```
        Total tests: 2. Passed: 2. Failed: 0. Skipped: 0.
        Test Run Failed.
        ```

        In case you get the following error message, just ignore it:

        ```
        System.IO.FileNotFoundException: Unable to find tests for .../prose/Transformation.Conflict/ICSE'21_Merge_Conflict/bin/Debug/netcoreapp2.1/ProseTutorial.dll. Make sure test project has a nuget reference of package "Microsoft.NET.Test.Sdk" and framework version settings are appropriate. Rerun with /diag option to diagnose further.
        ```

1.  Running the console application

    +   Open the VS Code terminal: View -> Integrated Terminal (or Ctrl+\`)

        ```
        cd Transformation.Conflict/ICSE'21_Merge_Conflict
        dotnet run
        ```

## Part 1: Understanding the Domain Specific Language (DSL).


1. Go to the folder "MergeConflictResolution".
    ```
    cd MergeConflictResolution
    ```

    The solution 'MergeConflictResolution' contains two projects. The first project, MergeConflictResolution, contains the components that you should implement to perform inductive synthesis using PROSE:

    + MergeConflictResolution/LanguageGrammar.grammar - The grammar of our DSL.

    + MergeConflictResolution/Semantics.cs - The semantics of our DSL.

    + MergeConflictResolution/WitnessFunctions.cs - Witness functions used by Prose to learn programs using examples.

    Additionally, it contains a console application where you can try out the synthesis system that you have created on new tasks (by  providing input-output examples to  synthesize a program for a task and then checking the behavior of the synthesized program on    new test inputs). The second project, MergeConflictResolution.Tests, contains the test cases that we will use to guide the tutorial (see Tests.cs).

2. Open the Test.cs file and run the testcase "LearnExample1c1d" (Right-Click on the test case -> Run Tests). It should pass. This test case learns a program from an example and test whether the actual output of the program matches the expected one.

    + Here, the header `cursor_type.mojom-shared.h` and `cursor_type.mojom-blink.h` appear both in the forked and main branches and the         developer excluded the one in the main branch.

3. Now that the test case is passing, you can also check the synthesized program and provide more inputs to these programs as shown in "TestExample1c1d".

    + Open the Tests.cs file and provide more example.

    + Try to provide a new example as shown in "TestExample1c1d". Example:

        ```diff
        forked branch:
        - #include "base/logging.h";
        + #include "ui/base/anonymous_ui_base_features.h";
        main branch:
        + #include "ui/base/cursor/mojom/cursor_type.mojom-shared.h";
        ```

    4. Try to build more tests with the forked and main section of the conflicts. For these examples, we have not giving the actual file for input. However, you can try with more examples by converting the file into a IReadonlyList<Node> and giving the actual location of the repository to find the path of the header files and read the files.

## Part 2: Understanding Semantic Functions.

1. Go to the folder "MergeConflictResolution" and open "Semantics.cs".

2. For each operation described in the grammar, we have one semantic function and the details can be found in our paper.


## Part 3: Understanding Witness Functions.

1. Go to the folder "MergeConflictResolution" and open "WitnessFunctions.cs".

2. For each operation described in the semantics and rules (let rule), we have one witness function and the details can be found in our paper.


## Part 5: Understanding Ranking.

1. Go to the folder "MergeConflictResolution" and open "RankingScore.cs".
2. We produce the ranking by expanding the abstract syntax tree (AST) of the program and give the program which is smaller and suffice to express the examples given as input.
