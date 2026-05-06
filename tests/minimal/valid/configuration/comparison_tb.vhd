entity comparison_tb is
end comparison_tb;

architecture sim of comparison_tb is

  signal a : integer;
  signal b : integer;
  signal q : boolean;

  component comparison
    port (
      a : in integer;
      b : in integer;
      q : out boolean
    );
  end component;

begin

  DUT : comparison
  port map (
    a => a,
    b => b,
    q => q
  );

  SEQUENCER_PROC : process      

    procedure test (x : integer; y : integer) is
    begin
      a <= x;
      b <= y;
      wait for 10 ns;
      
      report "a = " & integer'image(a)
        & ", b = " & integer'image(b)
        & ", q = " & boolean'image(q);
    end procedure;

  begin

    test(10, 5);
    test(5, 5);
    test(5, 10);

    wait;
  end process;

end architecture;