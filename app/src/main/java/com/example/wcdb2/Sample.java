package com.example.wcdb2;

import com.tencent.wcdb.WCDBField;
import com.tencent.wcdb.WCDBTableCoding;

@WCDBTableCoding
public class Sample {
    @WCDBField
    public int id;
    @WCDBField
    public String content;
}
