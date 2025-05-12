#ifndef WRITE_IR_H
#define WRITE_IR_H

#define WRITE_ASSIGN_VAR(var_struct)                                                                            \
    fprintf (IR_file, "\tGYPSI(88_tmp%lu, VAR, %lld)            # var \"%s\" to tmp\n",                          \
             *tmp_var_counter, var_struct.index, var_struct.variable);                                          \
    (*tmp_var_counter)++;

#define WRITE_ASSIGN_NUM(number)                                                                                \
    fprintf (IR_file, "\tGYPSI(88_tmp%lu, NUM, %lf)            # num to tmp\n", *tmp_var_counter, number);      \
    (*tmp_var_counter)++;

#define WRITE_ASSIGN_TMP(number)                                                                                \
    fprintf (IR_file, "\tGYPSI(88_tmp%lu, TMP, 88_tmp%lu)      # tmp to tmp\n", *tmp_var_counter, number);      \
    (*tmp_var_counter)++;

#define WRITE_ASSIGN_TMP_TO_VAR(var_struct, tmp_index)                                                          \
    fprintf (IR_file, "\tGYPSI(%lld, TMP, 88_tmp%lu)            # tmp to var \"%s\"\n",                          \
             var_struct.index, tmp_index, var_struct.variable);

#define WRITE_ASSIGN_ARG_TO_VAR(var_struct, arg_index)                                                          \
    fprintf (IR_file, "\tGYPSI(%lld, TMP, 14_arg%lu)            # arg to var \"%s\"\n",                          \
             var_struct.index, arg_index, var_struct.variable);

#define WRITE_ASSIGN_RES(var_struct)                                                                              \
    fprintf (IR_file, "\tGYPSI(%lld, TMP, 88_tmp%lu)              # result to variable \"%s\"\n",                          \
             var_struct.index, *tmp_var_counter - 1, var_struct.variable);

#define WRITE_ASSIGN_RES_TO_ARG(arg_index)                                                                      \
    fprintf (IR_file, "\tGYPSI(14_arg%lu, TMP, 88_tmp%lu)       # result to argument in function\n",              \
             arg_index, *tmp_var_counter - 1);

#define WRITE_ASSIGN_TMP_TO_ARG(arg_index, tmp_index)                                                           \
    fprintf (IR_file, "\tGYPSI(14_arg%lu, TMP, 88_tmp%lu)       # tmp to argument in function\n",                 \
             arg_index, tmp_index);

#define WRITE_ASSIGN(operation, operand1, operand2)                                                             \
    fprintf (IR_file, "\tGYPSI(88_tmp%lu, %s, 88_tmp%lu, 88_tmp%lu)\n",                                           \
             *tmp_var_counter, operation, operand1, operand2);                                                  \
    (*tmp_var_counter)++;


#define WRITE_CALL(function, cnt_args)                                                                          \
    fprintf (IR_file, "CALL_PENIS(88_tmp%lu, func_%s_%lu, %lu)\n",                                              \
             *tmp_var_counter, function, cnt_args, cnt_args);                                                   \
    (*tmp_var_counter)++;

#define WRITE_FUNC(function, cnt_args)                                                                          \
    fprintf (IR_file, "\nPENIS(func_%s_%lu, %lu)\n\n",                                                          \
             function, cnt_args, cnt_args);

#define WRITE_RET(ret_tmp_index)                                                                                \
    fprintf (IR_file, "\nKILL_PENIS(88_tmp%lu)\n\n", ret_tmp_index);


#define WRITE_LABEL(label_num, meaning)                                                                         \
    fprintf (IR_file, "\nFIFT(label%lu)                          # " meaning "\n",                              \
             label_num);


#define WRITE_COND_JMP(label_num, tmp_num, meaning)                                                             \
    fprintf (IR_file, "\nENTER(label%lu, 88_tmp%lu)              # " meaning "\n",                              \
             label_num, tmp_num);

#define WRITE_JMP(label_num, meaning)                                                                           \
    fprintf (IR_file, "\nENTER(label%lu)                         # " meaning "\n",                              \
             label_num);

#endif // WRITE_IR_H
