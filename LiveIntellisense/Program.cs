using System;
using System.IO;
using System.Threading;
using System.Diagnostics;

namespace LiveIntellisense
{
    class Program
    {
        // Config
        static string inputDir = "Input";
        static string intellisenseProg = "CSharpIntellisense.exe";
        static int timeout = 1000;

        static void Main(string[] args)
        {
            try
            {
                Engine engine = new Engine(inputDir, timeout, intellisenseProg);
                var thread = new Thread(() => engine.Run());
                thread.IsBackground = true;
                thread.Start();

                // Enter to quit the program.
                // while (Console.ReadKey().Key != ConsoleKey.Enter) ;

                // Any key to exit
                Console.ReadKey();
            }
            catch (Exception e)
            {
                Console.WriteLine("Program terminated because of errors.");
                Console.WriteLine(e.Message);
                Console.ReadKey();
            }
        }

    }
}
