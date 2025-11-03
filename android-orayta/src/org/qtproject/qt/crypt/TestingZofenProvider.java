package org.qtproject.qt.crypt;

public class TestingZofenProvider implements IZofenProvider
{       
        String magicString = "SECRET!";
        
        public String getZofen() 
        {
                return magicString;
        }

        public boolean isProviderAvailable() 
        {
                return true;
        }
}