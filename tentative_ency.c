int ency_nlines_cb(Ihandle* h) {
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( h, "USERDATA" );
  return userdata->size;
}

int ency_ncols_cb(Ihandle* h) {return 2;}

int ency_height_cb(Ihandle* h, int i) {return NXPIUP_ENCY_HEIGHT;}

int ency_width_cb(Ihandle* h, int j) { return NXPIUP_ENCY_WIDTH; }

int ency_mouseclick_cb(Ihandle* h, int b, int p, int i, int j, int x, int y, char* r)
{
  if( IUP_BUTTON1 == b ){
    if( p ){
      S_LineClicked = i;
    }
    else{
      if( i == S_LineClicked ){
	ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( h, "USERDATA" );
	userdata->selected = ( i == userdata->selected ) ? -1 : i;
	printf( "ENCY Click sel=%d, at=(%d, %d)\n", userdata->selected, x, y );
	IupUpdate( (Ihandle *) IupGetAttribute( h, "CANVAS" ) );
      }
      S_LineClicked = -1;
    }
  }
  return IUP_DEFAULT;
}

int ency_draw_cb(Ihandle* h, int i, int j, int xmin, int xmax, int ymin, int ymax, cdCanvas* canvas) 
{
  /* int xm = (xmax + xmin) / 2; */
  /* int ym = (ymax + ymin) / 2; */
  /* char buffer[64]; */

  /* cdCanvasForeground(canvas, cdEncodeColor( */
  /*   (unsigned char)(i*20),  */
  /*   (unsigned char)(j*100),  */
  /*   (unsigned char)(i+100) */
  /* )); */

  /* cdCanvasBox(canvas, xmin, xmax, ymin, ymax); */
  /* cdCanvasTextAlignment(canvas, CD_CENTER); */
  /* cdCanvasForeground(canvas, CD_BLACK); */
  /* sprintf(buffer, "(%02d, %02d)", i, j); */
  /* cdCanvasText(canvas, xm, ym, buffer); */
  char buf[NXPIUP_TEMP_BUFSIZE] = {0};
  char val[NXPIUP_TEMP_BUFSIZE] = {0};
  int  font_height, xm, ym, item;
  long int fgcolor, bgcolor;
  int style;
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( h, "USERDATA" );
  sign_rec_ptr sign;

  cdCanvasFont( canvas, "Times", CD_PLAIN, 10 );
  cdCanvasGetFontDim( canvas, NULL, &font_height, NULL, NULL );
  xm = xmin; ym = ymin + ( ymax - ymin - font_height)/2;
  sign = userdata->seq[ i-1 ];
  if( sign ){
    cdCanvasTextAlignment(canvas, CD_BASE_LEFT);
    switch(j){
    case 1:
      sprintf( buf, "%.32s", sign->str );
      break;
    case 2:
      nxpiup_ency__valuestr( sign, val );
      sprintf( buf, "%.16s", val );
      xm += 5;
      break;
    }
    /* printf( "\tREDRAW %s, %d, %d; min=(%d, %d), max=(%d, %d), at=(%d, %d)\n", */
    /* 	    buf, i, j, xmin, ymin, xmax, ymax, xm, ym ); */
    if( i == userdata->selected ){
      style	= cdCanvasInteriorStyle( canvas, CD_SOLID );
      bgcolor	= cdCanvasForeground( canvas, CD_GRAY );
      cdCanvasBox( canvas, xmin, ymin, xmax, ymax );
      bgcolor	= cdCanvasForeground( canvas, bgcolor );
      style	= cdCanvasInteriorStyle( canvas, style );
    }
    fgcolor = cdCanvasForeground(canvas, nxpiup_ency__textcolor( sign ));
    cdCanvasText(canvas, xm, ym, buf);
    fgcolor = cdCanvasForeground( canvas, fgcolor );
  }
  return IUP_DEFAULT;
}

int ency_destroy_cb( Ihandle *ih ){
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  if( userdata ) nxpiup_ency__freerec( userdata );
  return IUP_DEFAULT;
}


void nxpiup_dlgency( const char *ency_title, const char *ency_handle, sign_rec_ptr top ){
  Ihandle *dlg = IupGetHandle( ency_handle );
  if( !dlg ){
    sign_rec_ptr sign = top;
    if( !sign ) return;
    // Initfill encyrecord
    short item = 0;
    sign = top;
    while( sign ){
      item += 1;
      sign = sign->next;
    }
    ency_rec_ptr userdata = nxpiup_ency__newrec( item );
    item = 0;
    sign = top;
    while( sign ){
      userdata->seq[ item++ ] = sign;
      sign = sign->next;
    }
    /* printf( "QSORT pre\n" ); */
    /* for( short i=0; i<userdata->size; i++ ){ printf( "\t%s\n", userdata->seq[i]->str ); } */
    qsort( (void *) userdata->seq, (size_t) userdata->size, sizeof(sign_rec_ptr), nxpiup_ency__compare );
    /* printf( "QSORT post\n" ); */
    /* for( short i=0; i<userdata->size; i++ ){ printf( "\t%s\n", userdata->seq[i]->str ); } */
    //
    Ihandle *encyh = IupCells();
    IupSetAttribute( encyh, "USERDATA", (char *)userdata );
    IupSetAttribute( encyh, "BOXED", "FALSE" );
    IupSetCallback(encyh, "MOUSECLICK_CB", (Icallback)ency_mouseclick_cb);
    IupSetCallback(encyh, "DRAW_CB", (Icallback)ency_draw_cb);
    IupSetCallback(encyh, "WIDTH_CB", (Icallback)ency_width_cb);
    IupSetCallback(encyh, "HEIGHT_CB", (Icallback)ency_height_cb);
    IupSetCallback(encyh, "NLINES_CB", (Icallback)ency_nlines_cb);
    IupSetCallback(encyh, "NCOLS_CB", (Icallback)ency_ncols_cb);
    IupSetCallback(encyh, "DESTROY_CB", (Icallback)ency_destroy_cb);

    char buf[NXPIUP_TEMP_BUFSIZE] = {0};
    sprintf( buf, "%s_view", ency_handle );
    IupSetHandle( buf, encyh );
    //
    Ihandle *ency_vbox = IupVbox( IupFrame( encyh ), NULL );
    IupSetAttribute( ency_vbox, "MARGIN","10x10" );
    
    dlg = IupDialog( ency_vbox );
    sprintf( buf, "Encyclopedia %s", ency_title );
    IupSetAttribute( dlg, "TITLE", buf );
    IupSetAttribute( dlg,"RASTERSIZE","420x400" );
    /* IupSetAttribute( dlg, "EXPANDCHILDREN", "YES"); */
    IupSetHandle( ency_handle, dlg );
  }
  IupShow( dlg );
}
