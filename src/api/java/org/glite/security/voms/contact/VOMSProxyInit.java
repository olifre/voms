/*********************************************************************
 *
 * Authors: 
 *      Andrea Ceccanti - andrea.ceccanti@cnaf.infn.it 
 *          
 * Copyright (c) 2006 INFN-CNAF on behalf of the EGEE project.
 * 
 * For license conditions see LICENSE
 *
 * Parts of this code may be based upon or even include verbatim pieces,
 * originally written by other people, in which case the original header
 * follows.
 *
 *********************************************************************/
package org.glite.security.voms.contact;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.apache.log4j.Logger;
import org.glite.security.voms.PKIVerifier;
import org.glite.security.voms.ac.AttributeCertificate;
import org.globus.gsi.GlobusCredential;


/**
 *
 * This class implements the voms-proxy-init functionality.
 * 
 * @author Andrea Ceccanti
 *
 */
public class VOMSProxyInit {

    private static final Logger log = Logger.getLogger( VOMSProxyInit.class );
    
    private static VOMSProxyInit instance;
    
    private VOMSServerMap serverMap;
    private UserCredentials userCredentials;
    private VOMSProtocol protocol = VOMSProtocol.instance();
    
    private String proxyOutputFile = File.separator+"tmp"+File.separator+"x509up_u_"+System.getProperty( "user.name" ); 
    
    private int proxyLifetime = VOMSProxyBuilder.DEFAULT_PROXY_LIFETIME;
    
    private int proxyType = VOMSProxyBuilder.DEFAULT_PROXY_TYPE;
    
    private int delegationType = VOMSProxyBuilder.DEFAULT_DELEGATION_TYPE;

    private String policyType = null;

    public VOMSProxyInit(String privateKeyPassword){
    
        try {
            
            serverMap = VOMSESFileParser.instance().buildServerMap();
            
            userCredentials = UserCredentials.instance(privateKeyPassword);
            
        } catch ( IOException e ) {
        
            log.error( "Error parsing vomses files: "+e.getMessage() );
            if (log.isDebugEnabled())
                log.error(e.getMessage(),e);
            
            throw new VOMSException(e);
        }
        
        
    }
    
    public static VOMSProxyInit instance(String privateKeyPassword){
        if (instance == null)
            instance = new VOMSProxyInit(privateKeyPassword);
        
        return instance;
    }
    
    public static VOMSProxyInit instance(){
        if (instance == null)
            instance = new VOMSProxyInit(null);
        
        return instance;
    }
    
    public void addVomsServer(VOMSServerInfo info){
        
        serverMap.add( info );
        
    }
    
    public synchronized AttributeCertificate getVomsAC(VOMSRequestOptions requestOptions){
        
        if (requestOptions.getVoName() == null)
            throw new VOMSException("Please specify a vo name to create a voms ac.");
        
        Set servers = serverMap.get( requestOptions.getVoName());
        
        Iterator serverIter = servers.iterator();
        
        while(serverIter.hasNext()){
            
            VOMSServerInfo serverInfo = (VOMSServerInfo) serverIter.next();
            
            try{
            
                VOMSResponse response = contactServer( serverInfo, requestOptions );
                
                if (!response.hasErrors()){
                    
                    AttributeCertificate ac = VOMSProxyBuilder.buildAC(response.getAC());
                    log.info( "Got AC from VOMS server "+serverInfo.compactString() );
                    
                    if (log.isDebugEnabled()){
                        
                        try {
                            log.debug( "AC validity period:\nNotBefore:"+ac.getNotBefore()+"\nNotAfter:"+ac.getNotAfter() );
                     
                        } catch ( ParseException e ) {
                            
                            log.error( e.getMessage(),e );
                            e.printStackTrace();
                        }
                        
                    }
                    
                    return ac;
                }
                
                log.error( "Got error response from VOMS server "+serverInfo.compactString() );
                logErrorMessages( response );
                
            }catch(VOMSException e){
                
                log.error(e.getMessage());
                if (log.isDebugEnabled()){
                    log.error(e.getMessage(),e);
                }
                
                if (serverIter.hasNext())
                    continue;
                
                throw(e);
            }
        }
        
        return null;            
    }
    
    public void validateACs(List ACs){
        
        if (ACs.isEmpty())
            throw new VOMSException("Cannot validate an empty list of Attribute Certificates!");
        
        log.debug("AC Validation started at: "+ new Date(  ));
        
        PKIVerifier verifier;
        
        try {
        
            verifier = new PKIVerifier();
        
        } catch ( Exception e ) {
            
            log.error("Error instantiating PKIVerifier: "+e.getMessage());
            
            if (log.isDebugEnabled())
                log.error(e.getMessage(),e);
            throw new VOMSException("Error instantiating PKIVerifier: "+e.getMessage(),e);
            
        }
        
        Iterator i = ACs.iterator();
        
        while(i.hasNext()){
            
            AttributeCertificate ac = (AttributeCertificate)i.next();
            
            if (!verifier.verify( ac ))
                i.remove();    
        }
        
        log.debug("AC Validation ended at: "+ new Date(  ));
        
    }
    public synchronized GlobusCredential getVomsProxy(){
        
        return getVomsProxy( null );
        
    }
    
    
    protected GlobusCredential getGridProxy(){
        
        GlobusCredential proxy = VOMSProxyBuilder.buildProxy( userCredentials, proxyLifetime, proxyType);
        
        try{
            
            saveProxy( proxy );
            return proxy;
            
        }catch ( FileNotFoundException e ) {
            
            log.error("Error saving proxy to file "+proxyOutputFile+":"+e.getMessage());
            if (log.isDebugEnabled())
                log.error(e.getMessage(),e);
            
            throw new VOMSException("Error saving proxy to file "+proxyOutputFile+":"+e.getMessage(),e);
        }
        
    }
    public synchronized GlobusCredential getVomsProxy(Collection listOfReqOptions){
        
        
        if (listOfReqOptions == null)
            return getGridProxy();
        
        if (listOfReqOptions.isEmpty())
            throw new VOMSException("No request options specified!");
        
        Iterator i = listOfReqOptions.iterator();
        
        List ACs = new ArrayList();
        
        while (i.hasNext()){
            
            VOMSRequestOptions options = (VOMSRequestOptions)i.next();
            
            if (options.getVoName() == null)
                throw new VOMSException("Please specify a vo name to create a voms proxy.");
            
            AttributeCertificate ac = getVomsAC( options );
            
            ACs.add(ac);
            
        }
        
        validateACs( ACs );
        
        if (ACs.isEmpty())
            throw new VOMSException("AC validation failed!");
        
        log.info( "ACs validation succeded." );
        
        GlobusCredential proxy = VOMSProxyBuilder.buildProxy( userCredentials, 
                                                              ACs, proxyLifetime, 
                                                              proxyType, 
                                                              delegationType,
                                                              policyType);
        
        try{
            
            saveProxy( proxy );
            return proxy;
            
        }catch ( FileNotFoundException e ) {
            
            log.error("Error saving proxy to file "+proxyOutputFile+":"+e.getMessage());
            if (log.isDebugEnabled())
                log.error(e.getMessage(),e);
            
            throw new VOMSException("Error saving proxy to file "+proxyOutputFile+":"+e.getMessage(),e);
        }
        
        
    }
    
    
    private void saveProxy(GlobusCredential credential) throws FileNotFoundException{
        
        if (proxyOutputFile != null){
            VOMSProxyBuilder.saveProxy( credential, proxyOutputFile );
            log.info( "Proxy saved in :"+proxyOutputFile);
        }
        
    }
    
    private void logErrorMessages(VOMSResponse response){
        
        VOMSErrorMessage[] msgs = response.errorMessages();
        
        for ( int i = 0; i < msgs.length; i++ ) {
            log.error(msgs[i]);
        }
        
        
    }
    protected VOMSResponse contactServer(VOMSServerInfo sInfo, VOMSRequestOptions reqOptions) {
        
        log.info("Contacting server "+sInfo.compactString() );
        VOMSSocket socket;
        
        int gridProxyType = sInfo.getGlobusVersionAsInt();
        
        if (gridProxyType > 0)
            socket = VOMSSocket.instance( userCredentials, sInfo.getHostDn(), gridProxyType );
        else
            socket = VOMSSocket.instance( userCredentials, sInfo.getHostDn());
        
        try {
            
            socket.connect( sInfo.getHostName(), sInfo.getPort());
            
        } catch ( Exception e ) {
            
            log.error( "Error connecting to "+sInfo.compactString()+":"+e.getMessage() );
            
            if (log.isDebugEnabled())
                log.error(e.getMessage(),e);
            throw new VOMSException("Error connecting to "+sInfo.compactString()+":"+e.getMessage() ,e);
            
        } 
        
        VOMSResponse response;
        
        try {
            
            protocol.sendRequest( reqOptions, socket.getOutputStream());
            response = protocol.getResponse( socket.getInputStream() );
            
            socket.close();
            
            
        } catch ( IOException e ) {
            log.error( "Error communicating with server "+sInfo.getHostName()+":"+sInfo.getPort()+":"+e.getMessage() );
            
            if (log.isDebugEnabled())
                log.error(e.getMessage(),e);
            throw new VOMSException("Error communicating with server "+sInfo.getHostName()+":"+sInfo.getPort()+":"+e.getMessage(),e);
        }
        
        return response;
           
    }

    
    public String getProxyOutputFile() {
    
        return proxyOutputFile;
    }

    
    public void setProxyOutputFile( String proxyOutputFile ) {
    
        this.proxyOutputFile = proxyOutputFile;
    }

    
    public int getProxyLifetime() {
    
        return proxyLifetime;
    }

    
    public void setProxyLifetime( int proxyLifetime ) {
    
        this.proxyLifetime = proxyLifetime;
    }

    public int getProxyType() {
    
        return proxyType;
    }

    
    public void setProxyType( int proxyType ) {
    
        this.proxyType = proxyType;
    }

    
    public String getPolicyType() {
        return policyType;
    }

    public void setPolicyType( String policyType ) {
        this.policyType = policyType;
    }

    public int getDelegationType() {
    
        return delegationType;
    }

    
    public void setDelegationType( int delegationType ) {
    
        this.delegationType = delegationType;
    }
    
    
}