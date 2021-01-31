using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace AssemblyReader
{
    class Reader
    {
        public bool any;
        public string member;
        public string invoker;
        public string findtype;


        #region ConstIdentifiers
        private const string TYPE = "TYPE";
        private const string VAR = "VAR";
        private const string METHOD = "METHOD";
        private const string BASETYPES = "BASETYPES";

        private const string PRIVATE = "PRIVATE";
        private const string PROTECTED = "PROTECTED";
        private const string PUBLIC = "PUBLIC";
        private const string STATIC = "STATIC";

        private const string FALSE = "FALSE";
        private const string TRUE = "TRUE";
        #endregion

        public List<Assembly> assemblies = new List<Assembly>();

        public Reader(string assdirs, string asspaths)
        {
            // load assemblies from directories0
            if (!string.IsNullOrEmpty(assdirs))
            {
                var splitted = assdirs.Split(' ');
                foreach (var dir in splitted)
                {
                    DirectoryInfo di = new DirectoryInfo(dir);
                    foreach (var fi in di.GetFiles())
                        LoadAssembly(fi);

                }                
            }

            // load directories from files
            if (!string.IsNullOrEmpty(asspaths))
            {
                var splitted = asspaths.Split(' ');
                foreach (var file in splitted)
                    LoadAssembly(new FileInfo(file));

            }

        }

        private bool LoadAssembly(FileInfo fi)
        {
            bool success = false;
            if (fi.Extension.Equals("dll", StringComparison.CurrentCultureIgnoreCase))
            {
                try
                {
                    Assembly asm = Assembly.LoadFrom(fi.FullName);
                    assemblies.Add(asm);
                    success = true;
                }
                catch (Exception) { }
            }

            return success;
        }

        public void Run()
        {
            // ----- Find -----
            if (!string.IsNullOrEmpty(findtype))
                invoker = findtype;

            Type t = Type.GetType(invoker);
            if (t == null)
            {
                foreach (var asm in assemblies)
                {
                    t = asm.GetType(invoker);
                    if (t != null)
                        break;
                }

            }

            // ----- Found? -----
            if (!string.IsNullOrEmpty(findtype))
            {
                if ((findtype.Equals("System.Void")) // special type only for reflections - ignore!
                 || (t == null))
                {
                    Console.WriteLine(FALSE);
                    return;
                }

                else
                    Console.WriteLine(TRUE);

            }

            // ----- Print (all necessary info about this type to reconstruct it) -----

            if (t != null)
            {
                if (!string.IsNullOrEmpty(findtype))
                    PrintBaseTypes(t);

                bool showAny = any;
                if (any == false && !string.IsNullOrEmpty(findtype)) showAny = true;

                // ----- FIELDS -----
                var fields = new List<FieldInfo>();
                if (showAny)
                {
                    fields = t.GetFields().ToList();
                }
                else
                {
                    var f = t.GetField(member);
                    if (f != null) fields.Add(f);
                }

                foreach (var f in fields)
                    PrintField(f);

                // ----- PROPERTIES -----
                var properties = new List<PropertyInfo>();
                if (showAny)
                {
                    properties = t.GetProperties().ToList();
                }
                else
                {
                    var p = t.GetProperty(member);
                    if (p != null) properties.Add(p);
                }

                foreach (var p in properties)
                    PrintProperty(p);

                // ----- METHODS & CONSTRUCTORS -----
                var methods = new List<MethodBase>();
                if (showAny)
                {
                    foreach (var mi in t.GetMethods())
                        methods.Add(mi);
                    foreach (var ci in t.GetConstructors())
                        methods.Add(ci);
                }
                else
                {
                    var mi = t.GetMethod(member);
                    if (mi != null) methods.Add(mi);

                    // TODO get constructors
                }

                foreach (var m in methods)
                    PrintMethod(m);

                // TODO add nested types 
                //else if ((member.MemberType & MemberTypes.TypeInfo) > 0
                //    || (member.MemberType & MemberTypes.NestedType) > 0)
                //PrintType(member as TypeInfo);

            
            
                // warning !!! no support for extension methods
            }
        }


        public void PrintMethod(MethodBase mb)
        {
            // default constructor method (System.Object) - hide it
            if (mb.Name.Equals(".ctor"))
                return;

            // convention: set_, get_, op_ -> special metadata method - hide it
            if (mb.Name.StartsWith("get_")
             || mb.Name.StartsWith("set_")
             || mb.Name.StartsWith("op_"))
                return;



            Console.Write(METHOD + " ");

            Console.Write(mb.ToString());

            if (mb.IsPublic) Console.Write(" " + PUBLIC);
            else if (mb.IsFamily) Console.Write(" " + PROTECTED);
            else if (mb.IsPrivate) Console.Write(" " + PRIVATE);

            if (mb.IsStatic) Console.Write(" " + STATIC);

            Console.Write('\n');

        }

        public void PrintType(TypeInfo ti)
        {
            Console.Write(TYPE + " ");
            Console.Write(ti.ToString());
            Console.Write('\n');
        }

        public void PrintProperty(PropertyInfo pi)
        {
            Console.Write("VAR" + " ");
            Console.Write(pi.ToString());

            if (pi.GetMethod.IsPublic) Console.Write(" " + PUBLIC);
            else if (pi.GetMethod.IsFamily) Console.Write(" " + PROTECTED);
            else if (pi.GetMethod.IsPrivate) Console.Write(" " + PRIVATE);

            if (pi.GetMethod.IsStatic) Console.Write(" " + STATIC);

            Console.Write('\n');
        }

        public void PrintField(FieldInfo fi)
        {
            Console.Write(VAR + " ");

            Console.Write(fi.ToString());

            if (fi.IsPublic) Console.Write(" " + PUBLIC);
            else if (fi.IsFamily) Console.Write(" " + PROTECTED);
            else if (fi.IsPrivate) Console.Write(" " + PRIVATE);

            if (fi.IsStatic) Console.Write(" " + STATIC);

            Console.Write('\n');
        }

        public void PrintBaseTypes(Type t)
        {
            Console.Write(BASETYPES + " ");
            
            if (t.BaseType == null)
            {
                Console.Write('\n');
                return; // no base type -> this is System.Object
            }

            Console.Write(t.BaseType.FullName);

            foreach (var intf in t.GetInterfaces())
            {
                if (intf.Name.Contains("`"))
                    continue;
                Console.Write(" " + intf.FullName);
            }

            Console.Write('\n');
        }
    }
}
