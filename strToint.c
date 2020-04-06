#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int strToInt(char *buf);

int main(){
    char buf[1024];
    scanf("%s",&buf);
    int x = strToInt(buf);
    printf("%d",x);
    return 0;
}    
 int strToInt(char * buf){
	
	int length = strlen(buf);
	for(int i=0;i<length;i++){
		if(buf[i]>='A' && buf[i]<='z'){
			perror("Invalid String!");
			exit(1);
		}
	}
	char symbols[length];
	int numbers[length];

	int length_sim=0,length_num=0;
	int i=0;
	int resoult=0;
	while(i<length){
		if(buf[i]>='0' && buf[i]<='9'){
			int number=0;
			while(i<length && buf[i]>='0' && buf[i]<='9'){
					char sim[1];
					sim[0]=buf[i];
					number=number*10+atoi(sim);
					i++;
			}
			numbers[length_num++] = number;
			
		} else if(buf[i]=='('){
			char str[length];
			int j=0,count_op=1,count_cl=0;;
			i++;
			while(count_op!=count_cl){
				str[j]=buf[i];
				if(buf[i]=='(')
					count_op++;
				if(buf[i]==')')
					count_cl++;
				i++;
				j++;
			}	
			str[j-1]='\0';
			numbers[length_num++]=strToInt(str);
		}else{
			symbols[length_sim++]=buf[i++];
		}
		
	}
	int t=1;
	while(t==1){
		int b=0;
		for(int k=0;k<length_sim;k++){
			if(symbols[k]=='*' || symbols[k]=='/'){
				if(symbols[k]=='*'){
					numbers[k]=numbers[k]*numbers[k+1];
				}
				if(symbols[k]=='/'){
					numbers[k]=numbers[k]/numbers[k+1];
				}
				for(int j=k+1;j<length_num-1;j++)
					numbers[j]=numbers[j+1];
				length_num--;
				for(int j=k;j<length_sim-1;j++)
					symbols[j]=symbols[j+1];

				length_sim--;
				b=1;
				break;
			}
		}
		t=b;
	}
	t=1;
	while(t==1){
		int b=0;
		for(int k=0;k<length_sim;k++){
			if(symbols[k]=='+' || symbols[k]=='-'){
				if(symbols[k]=='+'){
					numbers[k]=numbers[k]+numbers[k+1];
				}
				if(symbols[k]=='-'){
					numbers[k]=numbers[k]-numbers[k+1];
				}
				for(int j=k+1;j<length_num-1;j++)
					numbers[j]=numbers[j+1];
				length_num--;
				for(int j=k;j<length_sim-1;j++)
					symbols[j]=symbols[j+1];
				length_sim--;
				b=1;
				break;
			}
		}
		t=b;
	}
	resoult=numbers[0];
	return resoult;
	
	
 } 


