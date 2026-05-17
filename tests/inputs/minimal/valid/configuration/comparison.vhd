entity comparison is
  port (
    a : in integer;
    b : in integer;
    q : out boolean
  );
end comparison;

architecture rtl of comparison is
begin

  q <= a = b;

end architecture;