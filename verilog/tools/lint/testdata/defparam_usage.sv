module defparam_usage #(parameter int MY_PARAM = 0);
endmodule;

module foo;
  // defparam shouldn't be used, this should trigger the forbid-defparam rule
  defparam p0.MY_PARAM = 1;
  defparam_usage p0();
endmodule
