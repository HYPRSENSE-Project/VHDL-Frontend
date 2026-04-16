architecture rtl of top is
begin
process(sel, a, b)
begin
  case sel is
    when '0' => y <= a;
    when others => y <= b;
  end case;
end process;
end;