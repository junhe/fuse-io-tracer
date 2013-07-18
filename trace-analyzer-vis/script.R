df.milcread = rdmine()

df.onlyread = subset(df.milcread, operation == "trc_read")



################## Uses facet #################
plot_gantt <- function (df)
{
df$pid = as.factor(df$pid)
themin = min(df$starttime)
sn = 1:nrow(df)
df = cbind(sn, df)
df$starttime = df$starttime - themin
df$endtime = df$endtime - themin
df$tail = df$offset + df$length
ggplot(df, aes()) +
    geom_segment(aes(x=sn, xend=sn, y=offset, yend=tail), size=1) +
    xlab("Request Number") +
	ylab("Initial Offset (bytes)") +
	theme_bw() +
	opts(panel.grid.minor = element_blank()) +
	opts(panel.grid.major = element_blank()) +
	opts(axis.title.y=element_text(vjust=0.15))+
	facet_wrap(~pid, ncol=4)
}	 
plot_gantt(df.onlyread)




############# look at each pid  ###########
plot_gantt <- function (df)
{
	df$pid = as.factor(df$pid)
	themin = min(df$starttime)
	sn = 1:nrow(df)
	df = cbind(sn, df)
	df$starttime = df$starttime - themin
	df$endtime = df$endtime - themin
	df$tail = df$offset + df$length

	for ( mypid in levels(df$pid) ) {
		df.toplot = subset( df, pid == mypid )
		p = ggplot(df.toplot, aes()) +
			geom_segment(aes(x=sn, xend=sn, y=offset, yend=tail), size=5) +
			xlab("Request Number") +
			ylab("Initial Offset (bytes)") +
			theme_bw() +
			opts(panel.grid.minor = element_blank()) +
			opts(panel.grid.major = element_blank()) +
			opts(axis.title.y=element_text(vjust=0.15))+
			facet_wrap(~pid, ncol=4)
		print(p)
		readline()
	}
}	 
plot_gantt(df.onlyread)



############## look at the stride ################
plot_stride <- function(df)
{
	df$pid = as.factor(df$pid)
	themin = min(df$starttime)
	
	df$starttime = df$starttime - themin
	df$endtime = df$endtime - themin
	df$tail = df$offset + df$length
	# req #n's stride is n.offset - (n-1).offset
	#df$stride = df$offset - c(df$offset[1], df$offset[-length(df$offset)])
	#print(summary(as.factor(df$stride)))
	
	for ( mypid in levels(df$pid) ) {
		df.toplot = subset( df, pid == mypid )
		df.toplot$stride = df.toplot$offset - c(df.toplot$offset[1]-1, df.toplot$tail[-length(df.toplot$tail)])
		sn = 1:nrow(df.toplot)
		df.toplot = cbind(sn, df.toplot)
		p = ggplot(df.toplot, aes()) +
			geom_point(aes(x=sn, y=stride, color=factor(stride))) +
			xlab("Request Number") +
			ylab("Offset (bytes)") +
			theme_bw() +
			theme(panel.grid.minor = element_blank()) +
			theme(panel.grid.major = element_blank()) +
			theme(axis.title.y=element_text(vjust=0.15)) +
			facet_wrap(~pid)
		print(p)
		print(summary(factor(sort(df.toplot$stride))))
		readline()
	}	
}
plot_stride(df.onlyread)


############# look at each pid  ###########
plot_gantt <- function (df)
{
	df$pid = as.factor(df$pid)
	themin = min(df$starttime)
	sn = 1:nrow(df)
	df = cbind(sn, df)
	df$starttime = df$starttime - themin
	df$endtime = df$endtime - themin
	df$tail = df$offset + df$length

	for ( mypid in levels(df$pid) ) {
		df.toplot = subset( df, pid == mypid )
		p = ggplot(df.toplot, aes()) +
			geom_segment(aes(x=offset, xend=tail, y=sn, yend=sn), size=1) +
			facet_wrap(~pid, ncol=4)
		print(p)
		readline()
	}
}	 
plot_gantt(df.onlyread)



merge_contiguous <- function ( df )
{
      #df = subset(df, filename == "/home/manio/workdir/plfs/pagoda/data/testfile1")
	 df = df
     rows = nrow(df)
     i=1
     merged = NULL

     for ( i in 2:rows ) {
          if ( is.na(df[i,]$offset) == FALSE ) {
               merged = df[i,]
               break
          }
     }
     #print(merged)
     i = 1
     for ( j in (i+1):rows ) {
          # skip NA
          if ( is.na(df[j,]$offset) ) {
               next
          }
          #print("---------")
          #print(c(i,j))
          #print(df[j,]$offset)
          #print(c(merged[i,]$offset, merged[i,]$length))
          #print(df[j,]$offset == (merged[i,]$offset + merged[i,]$length))
          if ( df[j,]$offset == (merged[i,]$offset + merged[i,]$length) ) {
               merged[i,]$length = merged[i,]$length + df[j,]$length
               merged[i,]$endtime = df[j,]$endtime
          } else {
               merged = rbind(merged, df[j, ])
               i = i+1
          }
     }
     merged
}



############# New merge, print graph  ###########
merge_contiguous <- function (df)
{
	df$pid = as.factor(df$pid)
	themin = min(df$starttime)
	sn = 1:nrow(df)
	df = cbind(sn, df)
	df$starttime = df$starttime - themin
	df$endtime = df$endtime - themin
	df$tail = df$offset + df$length
	df = subset(df, operation=="trc_read")
	
	for ( mypid in levels(df$pid) ) {
		pdf = subset( df, pid == mypid )
		#pdf = pdf[with(pdf, order(offset)), ]
		pdf$stride = pdf$offset - c(pdf$offset[1]-1, pdf$tail[-length(pdf$tail)])
		pdf$seppre = !(pdf$stride == 0)
		pdf$seppre2 = c(pdf$seppre[-1],  TRUE)
		pdf = pdf[pdf$seppre == T | (pdf$seppre == F & pdf$seppre2 == T), ]
		pdf$tail2 = c(pdf$tail[-1], 0)
		pdf$seppre3 = c(pdf$seppre[-1], T)
		pdf = pdf[ pdf$seppre == T, ]
		pdf$tail = pdf$tail*pdf$seppre3 + pdf$tail2*!pdf$seppre3
		pdf$length = pdf$tail - pdf$offset
		pdf$stride = pdf$offset - c(pdf$offset[1]-1, pdf$tail[-length(pdf$tail)])
		pdf$stride2 = pdf$offset - c(0, pdf$offset[-length(pdf$offset)])
		pdf$sn = 1:nrow(pdf)   
		#print(pdf)
		p = ggplot(pdf[1:200,], aes()) +
			geom_segment(aes(x=offset, xend=tail+500000, y=sn, yend=sn), size=1) +
			coord_flip()
		print(p)
		#print(p)
		#readline()
		return(pdf)
	}
}	 
merge_contiguous(df)






############# time ~ offset, tail  ###########
plot_gantt <- function (df)
{
	df$pid = as.factor(df$pid)
	themin = min(df$starttime)
	sn = 1:nrow(df)
	df = cbind(sn, df)
	df$starttime = df$starttime - themin
	df$endtime = df$endtime - themin
	df$tail = df$offset + df$length

	for ( mypid in levels(df$pid) ) {
		df.toplot = subset( df, pid == mypid )
		p = ggplot(df.toplot, aes()) +
			geom_segment(aes(x=starttime, xend=endtime, y=offset, yend=tail), size=1) +
			xlab("Request Number") +
			ylab("Initial Offset (bytes)") +
			theme_bw() +
			opts(panel.grid.minor = element_blank()) +
			opts(panel.grid.major = element_blank()) +
			opts(axis.title.y=element_text(vjust=0.15))+
			facet_wrap(~pid, ncol=4)
		print(p)
		readline()
	}
}	 
plot_gantt(df.onlyread)

















pat = append( rep(868352, 31), 38617088 )


offsets = df2$offset[1:34]
stride1 = 1179648
stride2 = 38928384
cnt = 34
while ( cnt < 1024 ) {
	for ( i in 1:31 ) {
		offsets = append( offsets, tail(offsets, n=1)+stride1 )
		cnt = cnt + 1
		stopifnot( cnt == length(offsets) )
		stopifnot( cnt <= length(df$offset) )
		if(offsets[cnt] != df2$offset[cnt]) {
			print(cnt)
			print("small stride")
			print( c(offsets[cnt], df2$offset[cnt]) )
			readline()
		}
	}
	offsets = append( offsets, tail(offsets, n=1)+stride2 )
	cnt = cnt + 1
	if(offsets[cnt] != df2$offset[cnt]) {
	    print(cnt)
		print("big stride")
		print( c(offsets[cnt], df2$offset[cnt]) )
		readline()
	}
}

