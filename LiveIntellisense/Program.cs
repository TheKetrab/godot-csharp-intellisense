using System;
using System.IO;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LiveIntellisense
{
    class Program
    {
        // Config
        static string inputFile = "./input.cs";
        static string intellisenseProg = "./MyIntellisense.exe";
        static int timeout = 1000;
        static string text = "";

        static void Main(string[] args)
        {
            var thread = new Thread(() =>
            {
                while (true)
                {
                    try
                    {
                        DoIntelliSense();
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine(e.Message);
                    }

                    Thread.Sleep(timeout);
                }
            });

            thread.Start();

            // Enter to quit the program.
            while (Console.ReadKey().Key != ConsoleKey.Enter) ;
        }

        static void PrepareToPrint()
        {
            Console.Clear();
            Console.Write("Press <Enter> to exit... ");
        }

        static void DoIntelliSense()
        {
            string newText = File.ReadAllText(inputFile);

            // Must contains cursor!
            if (!newText.Contains("^|"))
                return;

            // Must be different than previous
            if (text.Equals(newText))
                return;

            PrepareToPrint();
            text = newText;

            using (Process process = new Process())
            {
                process.StartInfo = new ProcessStartInfo
                {
                    FileName = intellisenseProg,
                    Arguments = "\"" + text + "\"",
                    UseShellExecute = false,
                    RedirectStandardOutput = true
                };

                process.Start();

                StreamReader reader = process.StandardOutput;
                string output = reader.ReadToEnd();

                Console.WriteLine(output);
                process.WaitForExit();
            }

        }
    }
}
