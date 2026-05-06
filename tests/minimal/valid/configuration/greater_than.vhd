entity greater_than is
  port (
    a : in integer;
    b : in integer;
    q : out boolean
  );
end greater_than;

architecture rtl of greater_than is
begin

  q <= a > b;

end architecture;