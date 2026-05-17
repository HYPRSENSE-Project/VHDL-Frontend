configuration eq of comparison_tb is
  for sim
  end for;
end configuration;

configuration gt of comparison_tb is
  for sim
    for DUT : comparison
      use entity work.greater_than(rtl);
    end for;
  end for;
end configuration;

configuration lt of comparison_tb is
  for sim
    for DUT : comparison
      use entity work.greater_than(rtl)
        port map (
          a => b,
          b => a,
          q => q
        );
    end for;
  end for;
end configuration;