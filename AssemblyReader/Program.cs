﻿
namespace AssemblyReader
{
    class Program
    {
        // args:
        //
        // -assdirs [dirs]     - paths to directories to analyze all dlls
        // -asspaths [paths]   - paths to concrete dlls
        // -invoker [typename]
        // -member [membername]  - concrete member of invoker
        // -any                - all members of invoker
        //
        // -findtype [typename] - reconstruct type
        //
        // output
        // TYPE result MODIFIERS
        //
        //
        static void Main(string[] args)
        {
            // read input
            string assdirs = "";
            string asspaths = "";
            string invoker = "";
            string member = "";
            bool any = false;
            string findtype = "";

            for (int i=0; i<args.Length; i++)
            {
                if (args[i].Equals("-assdirs"))
                {
                    if (i+1 < args.Length)
                        assdirs = args[++i];
                }
                else if (args[i].Equals("-asspaths"))
                {
                    if (i+1 < args.Length)
                        asspaths = args[++i];
                }
                else if (args[i].Equals("-invoker"))
                {
                    if (i + 1 < args.Length)
                        invoker = args[++i];
                }
                else if (args[i].Equals("-member"))
                {
                    if (i + 1 < args.Length)
                        member = args[++i];
                }
                else if (args[i].Equals("-any"))
                {
                    any = true;
                }
                else if (args[i].Equals("-findtype"))
                {
                    if (i+1 < args.Length)
                        findtype = args[++i];
                }
            }

            Reader reader = new Reader(assdirs,asspaths)
            {
                any = any,
                member = member,
                invoker = invoker,
                findtype = findtype,
            };

            reader.Run();
            //Console.Read();
        }
    }
}
