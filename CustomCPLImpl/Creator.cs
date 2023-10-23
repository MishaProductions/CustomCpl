namespace CustomCPLImpl
{
    public unsafe class Creator
    {
        public delegate void* CreatePageDelegate();
        public static void* CreatePage()
        {
            MainPage pg = new MainPage();
            pg.CreateControl();
            pg.Show();
            return pg.Handle.ToPointer();
        }
    }
}