/*

        Part of the MSA assembler
        Copyright(C) 2001 Robert �stling

*/

void out_msg(char* s,int x) {
        if(x==0) errors++;
        else warnings++;
        if(quiet>=x) printf("%06ld: %s\n",linenr,s);
}

void build_address(t_address* a) {
        if(a->mod==3) {
                a->op_len=1;
                a->op[0]=(a->mod << 6) + (a->reg << 3) + a->rm;
                return;
        }
        if(a->mod<3) {
                a->op_len=1+a->mod;
                if(a->mod==0&&a->rm==6) {
                        a->op_len=3;
                }
        }
        a->op[0] = (a->mod << 6) + (a->reg << 3) + a->rm;
        if(a->mod==1) {
                a->op[1] = a->disp;
        } else
        if(a->mod==2||(a->mod==0&&a->rm==6)) {
                a->op[1] = (a->disp) & 0xff;
                a->op[2] = ((a->disp) & 0xff00) >> 8;
        }
}

int is_reg_8(char* s) {
        int i;
        for(i=0;i<8;i++)
                if(!stricmp(s,reg_8_table[i])) return i;
        return 8;
}

int is_reg_16(char* s) {
        int i;
        for(i=0;i<8;i++)
                if(!stricmp(s,reg_16_table[i])) return i;
        return 8;
}

int is_reg_seg(char* s) {
        int i;
        for(i=0;i<4;i++)
                if(!stricmp(s,reg_seg_table[i])) return i;
        return 8;
}

int get_type(char* s) {
        if(is_reg_8(s)!=8) return ACC_8+is_reg_8(s);
        if(is_reg_16(s)!=8) return ACC_16+is_reg_16(s);
        if(is_reg_seg(s)!=8) return SEG+is_reg_seg(s);
        if(!memicmp(s,"BYTE[",5)) return MEM_8;
        if(!memicmp(s,"WORD[",5)) return MEM_16;
        if(!memicmp(s,"[",1)) return MEM_16;
        return IMM;
}

void get_arg_types(char* s) {
        int i=0,j;
        char tmp[64];

        params=0;
        while(s[i]!=0) {
                j = 0;
                while(s[i]!=','&&s[i]!=0) {
                        tmp[j++]=s[i++];
                }
                if(tmp[j-1]==',') tmp[j-1]=0;
                tmp[j]=0;
                strcpy(param[params],tmp);
                param_type[params++]=get_type(tmp);
                if(s[i]==',') i++;
        }
}

void add_const(char* name, long int value) {
        int i;

        for(i=0;i<constants;i++) {
                if(!stricmp(constant[i].name,name)) {
                        if(value!=constant[i].value&&strcmp(name,"offset")&&name[0]!='$') {
                                out_msg("Constant changed",2);
                        }
                        constant[i].value = value;
                        return;
                }
        }
        strcpy(constant[constants].name,name);
        constant[constants].value = value;
        constants++;
}

int is_ident(char c) {
        if((c>='0'&&c<='9')||(c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'||c=='$'||c=='\'') return 1;
        return 0;
}

int is_numeric(char c) {
        if(c>='0'&&c<='9') return 1;
        return 0;
}

int is_hex_numeric(char c) {
        if((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')) return 1;
        return 0;
}

int is_space(char c) {
        if(c==' '||c==0xa||c==0xd||c==0x9) return 1;
        return 0;
}

long int get_number(char* s) {
        long int value=0;
        int i;

        strupr(s);
        if(s[1]=='X'&&s[0]=='0') {
                for(i=2;i<strlen(s);i++) {
                        if(!is_hex_numeric(s[i])) {
                                out_msg("Invalid digit",0);
                        }
                }
                sscanf(s,"0X%x",&value);
        } else {
                for(i=1;i<strlen(s);i++) {
                        if(!is_numeric(s[i])) {
                                out_msg("Invalid digit",0);
                        }
                }
                sscanf(s,"%ld",&value);
        }
        return value;
}

long int get_const(char* s) {
        int i,j;
        char tmp[32];
        char sign,found;
        long value=0,x;

        for(i=0;i<strlen(s);i++) {
                sign = '+';
                switch(s[i]) {
                        case '+':
                        case '-':
                        case '*':
                        case '/':
                        case '%':
                                sign=s[i++]; break;
                        default:
                                break;
                };
                j=0;
                while(is_ident(s[i])) tmp[j++] = s[i++];
                tmp[j]=0;
                i--;
                if(tmp[0]=='\'') {
                        x = tmp[1];
                } else if(is_numeric(tmp[0])) {
                        x = get_number(tmp);
                } else {
                        found = 0;
                        for(j=0;j<constants;j++) {
                                if(!stricmp(constant[j].name,tmp)) {
                                        x = constant[j].value;
                                        found = 1;
                                }
                        }
                        if(!found) {
                                out_msg("Unknown constant",2);
                                return 0x1000;
                        }
                }
                switch(sign) {
                        case '+':
                                value += x; break;
                        case '-':
                                value -= x; break;
                        case '*':
                                value *= x; break;
                        case '/':
                                value /= x; break;
                        case '%':
                                value = value % x; break;
                        default:
                                break;
                };
        }
        return value;
}

int get_address(t_address* a, char* s) {
        int i=0,j,k;
        byte seg_pre;
        word x;

        if(is_reg_8(s)<8) {
                a->rm = is_reg_8(s);
                a->mod = 3;
                return 0;
        } else
        if(is_reg_16(s)<8) {
                a->rm = is_reg_16(s);
                a->mod = 3;
                return 0;
        }

        while(s[i]!='['&&s[i]!=0) {
                i++;
        }
        if(s[i]==0) return 1;
        i++;
        j=i;

        if(s[i+2]==':') {
                if(!memicmp(&s[i],"CS",2)) seg_pre = 0x2e;
                else if(!memicmp(&s[i],"DS",2)) seg_pre = 0x3e;
                else if(!memicmp(&s[i],"ES",2)) seg_pre = 0x26;
                else if(!memicmp(&s[i],"SS",2)) seg_pre = 0x36;
                else {
                        out_msg("Syntax error",0);
                        seg_pre = 0x90;
                }
                for(k=old_outptr+4;k>old_outptr;k--)
                        outprog[k]=outprog[k-1];
                i+=3;
                j=i;
                outprog[old_outptr]=seg_pre;
                outptr++;
        }

        if(!memicmp(&s[i],"BX+SI",5)) {
                a->rm = 0;
                i += 5;
        } else
        if(!memicmp(&s[i],"BX+DI",5)) {
                a->rm = 1;
                i += 5;
        } else
        if(!memicmp(&s[i],"BP+SI",5)) {
                a->rm = 2;
                i += 5;
        } else
        if(!memicmp(&s[i],"BP+DI",5)) {
                a->rm = 3;
                i += 5;
        } else
        if(!memicmp(&s[i],"SI",2)) {
                a->rm = 4;
                i += 2;
        } else
        if(!memicmp(&s[i],"DI",2)) {
                a->rm = 5;
                i += 2;
        } else
        if(!memicmp(&s[i],"BP",2)) {
                a->rm = 6;
                i += 2;
        } else
        if(!memicmp(&s[i],"BX",2)) {
                a->rm = 7;
                i += 2;
        } else {
                a->mod = 0;
                a->rm = 6;
                s[strlen(s)-1]=0;
                a->disp = get_const(&s[j]);
                return 0;
        }

        if(s[i]=='+'||s[i]=='-'||s[i]=='*'||s[i]=='/'||s[i]=='%') {
                s[strlen(s)-1]=0;
                a->disp = get_const(&s[i]);
                if(a->disp<0x80) a->mod = 1;
                else a->mod = 2;
        } else {
                a->mod = 0;
        }

        if(a->rm==6&&a->mod==0) {
                a->disp = 0;
                a->mod = 1;
        }

        return 0;
}

int get_line(char* s) {
        int i=0;
        byte cf=0;

        while(prog[ptr]!=0x0d&&prog[ptr]!=0x0a&&prog[ptr]!=0) {
                if(prog[ptr]=='"') cf ^= 1;
                if(prog[ptr]==';'&&!cf) {
                        s[i++]=0;
                        ptr++;
                }
                else s[i++]=prog[ptr++];
        }
        s[i]=0;
        if(prog[ptr]==0) return 0;
        ptr++;
        if(prog[ptr]==0x0a||prog[ptr]==0x0d) ptr++;
        if(prog[ptr]==0) return 0;
        return 1;
}

void split(char* s)
{
        byte sf=1,cf=0;
        int i,wp=0;

        for(i=0;i<MAX_ARGS;i++) strcpy(arg[i],"");

        args=0;
        for(i=0;i<strlen(s);i++) {
                if(s[i]=='\"'&&cf==0) {
                        cf=1;
                        arg[args][wp++]=s[i];
                } else
                if(s[i]=='\"'&&cf==1) {
                        arg[args][wp++]='\"';
                        cf=0;
                        sf=0;
                } else
                if(s[i]!='\"'&&cf==1) {
                        arg[args][wp++]=s[i];
                } else
                if(s[i]==' '&&sf==1);
                else
                if(s[i]!=' '&&sf==1) {
                        sf=0;
                        arg[args][wp++]=s[i];
                } else
                if(s[i]!=' '&&sf==0) {
                        arg[args][wp++]=s[i];
                } else
                if(s[i]==' '&&sf==0) {
                        sf=1;
                        arg[args++][wp]=0;
                        wp=0;
                }
        }
        if(sf==0||cf==1) {
                arg[args++][wp]=0;
        }
}
