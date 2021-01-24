using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LiveIntellisense
{
    static class Printer
    {
        public static ConsoleColor typeColor = ConsoleColor.Green;
        public static ConsoleColor methodColor = ConsoleColor.Cyan;
        public static ConsoleColor currentArgColor = ConsoleColor.Red;
        public static ConsoleColor varColor = ConsoleColor.Yellow;

        private static string classIdent = "CLASS : ";
        private static string funcIdent = "FUNCTION : ";
        private static string varIdent = "VARIABLE : ";
        private static int indentation = 3;

        public static void Print(string str)
        {
            Console.WriteLine(str);
        }

        public static void Print(string str, ConsoleColor clr, string c = "\n")
        {
            var prev = Console.ForegroundColor;
            Console.ForegroundColor = clr;
            Console.Write(str + c);
            Console.ForegroundColor = prev;
        }

        public static void PrintIntellisenseOutput(string output)
        {
            output = output.Replace("\r\n", "\n");
            var lst = output.Split('\n').ToList();
            lst = lst.FindAll(s => 
                s.StartsWith(varIdent)
                || s.StartsWith(funcIdent)
                || s.StartsWith(classIdent)
            );

            foreach (var opt in lst)
            {
                if (opt.StartsWith(varIdent))
                    PrintVariable(opt);
                else if (opt.StartsWith(funcIdent))
                    PrintFunc(opt);
                else if (opt.StartsWith(classIdent))
                    PrintClass(opt);
            }

        }

        public static void PrintVariable(string variable)
        {
            variable = variable.Substring(varIdent.Length);
            PrintIndentation();
            Print(variable, varColor);
        }

        public static void PrintFunc(string func)
        {
            func = func.Substring(funcIdent.Length);
            PrintIndentation();

            int endArg = func.IndexOf("<|");
            if (endArg >= 0)
            {
                int startArg = func.IndexOf("|>");
                string beforeArg = func.Substring(0, startArg);
                int argLen = endArg - startArg - 2;
                string arg = func.Substring(startArg + 2, argLen);
                string afterArg = func.Substring(endArg + 2);

                Print(beforeArg, methodColor, "");
                Print(arg, currentArgColor, "");
                Print(afterArg, methodColor);
            }
            else
            {
                Print(func, methodColor);
            }

        }

        public static void PrintClass(string cls)
        {
            cls = cls.Substring(cls.Length);
            PrintIndentation();
            Print(cls, typeColor);
        }

        public static void PrepareConsole(string msg)
        {
            Console.Clear();
            Console.WriteLine(" > {0} < ", msg);
            Console.WriteLine("----- ----- -----");
        }

        public static void PrintIndentation()
        {
            Console.Write(new string(' ', indentation));
        }
    }
}
