## LiveIntellisense

LiveIntellisense is a program to manage files that simulates workspace directory and print options resolved for current context. In order to point at cursor placement type `^|`.


### Example 1:
```csharp
class C1
{
    public int x;
    public int y;

    System.Int64 DoSth(string str)
    {
        return 123456789;
    }

    void DoSth(int a, int b)
    {
        this.DoSth(^|)
    }
}
```
This one means: *What can I write as the first argument of DoSth method?*

Output:
```
FUNCTION DoSth(string str) : System.Int64
FUNCTION DoSth(int a, int b) : void
```

### Example 2:
```csharp
class C2
{
    public int x;
    public string y;
    public static object z;

    void DoSth(int a, int b)
    {
        this.^|
    }
}
```
This one means: *What are the non-static members of C2 class?*

Output:
```
VAR x : int
VAR y : string
FUNCTION DoSth(int a, int b) : void
```