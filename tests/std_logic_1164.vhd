package std_logic_1164 is
  type std_ulogic is ('u',  -- uninitialized
                      'x',  -- forcing  unknown
                      '0',  -- forcing  0
                      '1',  -- forcing  1
                      'z',  -- high impedance   
                      'w',  -- weak unknown
                      'l',  -- weak 0   
                      'h',  -- weak 1   
                      '-'   -- don't care
  );
  type std_ulogic_vector is array ( natural range <> ) of std_ulogic;
  
  function resolved ( s : std_ulogic_vector ) return std_ulogic;
  subtype std_logic is resolved std_ulogic;
  type std_logic_vector is array ( natural range <> ) of std_logic;

  subtype x01     is resolved std_ulogic range 'x' to '1'; -- ('x','0','1') 
  subtype x01z    is resolved std_ulogic range 'x' to 'z'; -- ('x','0','1','z') 
  subtype ux01    is resolved std_ulogic range 'u' to '1'; -- ('u','x','0','1') 
  subtype ux01z   is resolved std_ulogic range 'u' to 'z'; -- ('u','x','0','1','z') 

  function "and"  ( l : std_ulogic; r : std_ulogic ) return ux01;
  function "nand" ( l : std_ulogic; r : std_ulogic ) return ux01;
  function "or"   ( l : std_ulogic; r : std_ulogic ) return ux01;
  function "nor"  ( l : std_ulogic; r : std_ulogic ) return ux01;
  function "xor"  ( l : std_ulogic; r : std_ulogic ) return ux01;
  function "xnor" ( l : std_ulogic; r : std_ulogic ) return ux01;
  function "not"  ( l : std_ulogic                 ) return ux01;

  function "and"  ( l, r : std_logic_vector  ) return std_logic_vector;
  function "and"  ( l, r : std_ulogic_vector ) return std_ulogic_vector;

  function "nand" ( l, r : std_logic_vector  ) return std_logic_vector;
  function "nand" ( l, r : std_ulogic_vector ) return std_ulogic_vector;

  function "or"   ( l, r : std_logic_vector  ) return std_logic_vector;
  function "or"   ( l, r : std_ulogic_vector ) return std_ulogic_vector;

  function "nor"  ( l, r : std_logic_vector  ) return std_logic_vector;
  function "nor"  ( l, r : std_ulogic_vector ) return std_ulogic_vector;

  function "xor"  ( l, r : std_logic_vector  ) return std_logic_vector;
  function "xor"  ( l, r : std_ulogic_vector ) return std_ulogic_vector;

  function "xnor" ( l, r : std_logic_vector  ) return std_logic_vector;
  function "xnor" ( l, r : std_ulogic_vector ) return std_ulogic_vector;

  function "not"  ( l : std_logic_vector  ) return std_logic_vector;
  function "not"  ( l : std_ulogic_vector ) return std_ulogic_vector;

  function to_bit       ( s : std_ulogic;        xmap : bit := '0') return bit;
  function to_bitvector ( s : std_logic_vector ; xmap : bit := '0') return bit_vector;
  function to_bitvector ( s : std_ulogic_vector; xmap : bit := '0') return bit_vector;

  function to_stdulogic       ( b : bit               ) return std_ulogic;
  function to_stdlogicvector  ( b : bit_vector        ) return std_logic_vector;
  function to_stdlogicvector  ( s : std_ulogic_vector ) return std_logic_vector;
  function to_stdulogicvector ( b : bit_vector        ) return std_ulogic_vector;
  function to_stdulogicvector ( s : std_logic_vector  ) return std_ulogic_vector;

  function to_x01  ( s : std_logic_vector  ) return  std_logic_vector;
  function to_x01  ( s : std_ulogic_vector ) return  std_ulogic_vector;
  function to_x01  ( s : std_ulogic        ) return  x01;
  function to_x01  ( b : bit_vector        ) return  std_logic_vector;
  function to_x01  ( b : bit_vector        ) return  std_ulogic_vector;
  function to_x01  ( b : bit               ) return  x01;       

  function to_x01z ( s : std_logic_vector  ) return  std_logic_vector;
  function to_x01z ( s : std_ulogic_vector ) return  std_ulogic_vector;
  function to_x01z ( s : std_ulogic        ) return  x01z;
  function to_x01z ( b : bit_vector        ) return  std_logic_vector;
  function to_x01z ( b : bit_vector        ) return  std_ulogic_vector;
  function to_x01z ( b : bit               ) return  x01z;      

  function to_ux01  ( s : std_logic_vector  ) return  std_logic_vector;
  function to_ux01  ( s : std_ulogic_vector ) return  std_ulogic_vector;
  function to_ux01  ( s : std_ulogic        ) return  ux01;
  function to_ux01  ( b : bit_vector        ) return  std_logic_vector;
  function to_ux01  ( b : bit_vector        ) return  std_ulogic_vector;
  function to_ux01  ( b : bit               ) return  ux01;       

  function rising_edge  (signal s : std_ulogic) return boolean;
  function falling_edge (signal s : std_ulogic) return boolean;

  function is_x ( s : std_ulogic_vector ) return  boolean;
  function is_x ( s : std_logic_vector  ) return  boolean;
  function is_x ( s : std_ulogic        ) return  boolean;
end std_logic_1164;